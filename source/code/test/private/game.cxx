#include "game.hxx"
#include <ice/render/render_swapchain.hxx>
#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_frame.hxx>
#include <ice/gfx/gfx_pass.hxx>
#include <ice/gfx/gfx_stage.hxx>
#include <ice/world/world_manager.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_frame.hxx>

#include <ice/assert.hxx>
#include <ice/log.hxx>
#include <ice/stringid.hxx>
#include <ice/uri.hxx>

using ice::operator""_sid;
using ice::operator""_uri;

class ClearStage final : ice::gfx::GfxStage
{
public:
    ClearStage(
        ice::render::RenderSwapchain const& swapchain,
        ice::render::RenderDevice& render_device
    ) noexcept
        : _render_device{ render_device }
        , _render_swapchain{ swapchain }
        , _extent{ swapchain.extent() }
    {
        using namespace ice::render;

        RenderAttachment attachments[]{
            RenderAttachment{
                .format = swapchain.image_format(),
                .layout = ImageLayout::Present,
                .type = AttachmentType::SwapchainImage,
                .operations = {
                    AttachmentOperation::Load_Clear,
                    AttachmentOperation::Store_Store
                },
            },
            RenderAttachment{
                .format = ImageFormat::UNORM_D24_UINT_S8,
                .layout = ImageLayout::DepthStencil,
                .type = AttachmentType::DepthStencil,
            },
        };

        AttachmentReference references[]{
            AttachmentReference{
                .attachment_index = 0,
                .layout = ImageLayout::Color
            },
            AttachmentReference{
                .attachment_index = 1,
                .layout = ImageLayout::DepthStencil
            },
        };

        RenderSubPass subpasses[]{
            RenderSubPass{
                .color_attachments = { references + 0, 1 },
            },
            RenderSubPass{
                .color_attachments = { references + 0, 1 },
                .depth_stencil_attachment = references[1],
            }
        };

        SubpassDependency dependencies[]{
            SubpassDependency{
                .source_subpass = 0,
                .source_stage = PipelineStage::ColorAttachmentOutput,
                .source_access = AccessFlags::ColorAttachmentWrite,
                .destination_subpass = 1,
                .destination_stage = PipelineStage::ColorAttachmentOutput,
                .destination_access = AccessFlags::ColorAttachmentWrite,
            }
        };

        RenderPassInfo renderpass_info{
            .attachments = attachments,
            .subpasses = subpasses,
            .dependencies = dependencies,
        };

        _render_pass = render_device.create_renderpass(renderpass_info);

        ImageInfo image_info;
        image_info.type = ImageType::Image2D;
        image_info.format = ImageFormat::UNORM_D24_UINT_S8;
        image_info.usage = ImageUsageFlags::DepthStencilAttachment;
        image_info.width = _extent.x;
        image_info.height = _extent.y;
        _depth_stencil_image = _render_device.create_image(image_info, { });

        Image framebuffer_images[]{
            Image::Invalid,
            _depth_stencil_image
        };

        framebuffer_images[0] = swapchain.image(0);
        _framebuffers[0] = _render_device.create_framebuffer(
            _extent,
            _render_pass,
            framebuffer_images
        );

        framebuffer_images[0] = swapchain.image(1);
        _framebuffers[1] = _render_device.create_framebuffer(
            _extent,
            _render_pass,
            framebuffer_images
        );
    }

    ~ClearStage() noexcept
    {
        _render_device.destroy_framebuffer(_framebuffers[0]);
        _render_device.destroy_framebuffer(_framebuffers[1]);
        _render_device.destroy_image(_depth_stencil_image);
        _render_device.destroy_renderpass(_render_pass);
    }

    void record_commands(
        ice::render::CommandBuffer cmd_buffer,
        ice::render::RenderCommands& cmds
    ) noexcept override
    {
        static ice::vec4f const clear_color{ 0.2f };

        ice::u32 const framebuffer_idx = _render_swapchain.current_image_index();

        cmds.begin(cmd_buffer);
        cmds.begin_renderpass(cmd_buffer, _render_pass, _framebuffers[framebuffer_idx], _extent, clear_color);
        cmds.next_subpass(cmd_buffer, ice::render::SubPassContents::Inline);
        cmds.end_renderpass(cmd_buffer);
        cmds.end(cmd_buffer);
    }

private:
    ice::render::RenderDevice& _render_device;
    ice::render::RenderSwapchain const& _render_swapchain;
    ice::vec2u _extent;

    ice::render::RenderPass _render_pass;
    ice::render::Image _depth_stencil_image;
    ice::render::Framebuffer _framebuffers[2];
};

TestGame::TestGame(
    ice::Allocator& alloc,
    ice::Engine& engine,
    ice::UniquePtr<ice::EngineRunner> runner
) noexcept
    : _allocator{ alloc }
    , _engine{ engine }
    , _runner{ ice::move(runner) }
    , _gfx_device{ _runner->graphics_device() }
    , _render_device{ _gfx_device.device() }
    , _archetype_index{ ice::make_unique_null<ice::ArchetypeIndex>() }
    , _archetype_alloc{ ice::make_unique_null<ice::ArchetypeBlockAllocator>() }
    , _entity_storage{ ice::make_unique_null<ice::EntityStorage>() }
    , _world{ nullptr }
    , _stages{ _allocator }
{
    _archetype_index = ice::create_archetype_index(_allocator, { });
    _archetype_alloc = ice::make_unique<ice::ArchetypeBlockAllocator>(
        _allocator,
        _allocator
    );
    _entity_storage = ice::make_unique<ice::EntityStorage>(
        _allocator, _allocator,
        *_archetype_index,
        *_archetype_alloc
    );

    _world = _engine.world_manager().create_world(
        "default"_sid, _entity_storage.get()
    );

    ice::pod::array::push_back(
        _stages,
        _allocator.make<ClearStage>(
            _gfx_device.swapchain(),
            _render_device
        )
    );
}

TestGame::~TestGame() noexcept
{
    for (ice::gfx::GfxStage* stage : _stages)
    {
        _allocator.destroy(stage);
    }

    _engine.world_manager().destroy_world("default"_sid);

    _entity_storage = nullptr;
    _archetype_alloc = nullptr;
    _archetype_index = nullptr;
}

void TestGame::update() noexcept
{
    _runner->next_frame();

    ice::EngineFrame& frame = _runner->current_frame();
    ice::gfx::GfxFrame& gfx_frame = _runner->graphics_frame();
    ice::gfx::GfxPass* default_pass = gfx_frame.get_pass("default"_sid);
    ICE_ASSERT(default_pass != nullptr, "The 'default' graphics pass, does not exist!");

    for (ice::gfx::GfxStage* stage : _stages)
    {
        default_pass->add_stage(
            "default.clear"_sid,
            stage,
            { }
        );
    }
}
