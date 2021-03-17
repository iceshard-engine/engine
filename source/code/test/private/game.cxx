#include "game.hxx"
#include <ice/render/render_swapchain.hxx>
#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_frame.hxx>
#include <ice/gfx/gfx_pass.hxx>
#include <ice/gfx/gfx_stage.hxx>
#include <ice/world/world_manager.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_frame.hxx>

#include <ice/asset.hxx>
#include <ice/asset_system.hxx>

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
        ice::AssetSystem& asset_system,
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
                .operations = {
                    AttachmentOperation::Load_Clear
                }
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

        ice::render::ResourceSetLayoutBinding bindings[]{
            ice::render::ResourceSetLayoutBinding{
                .binding_index = 0,
                .resource_count = 1,
                .resource_type = ResourceType::UniformBuffer,
                .shader_stage_flags = ShaderStageFlags::FragmentStage | ShaderStageFlags::VertexStage
            }
        };

        _nomaterial_layout = _render_device.create_resourceset_layout(
            bindings
        );
        _render_device.create_resourcesets(
            ice::Span<ice::render::ResourceSetLayout const>{ &_nomaterial_layout, 1 },
            ice::Span<ice::render::ResourceSet>{ &_empty_material, 1 }
        );

        _opaque_layout = _render_device.create_pipeline_layout(
            PipelineLayoutInfo{
                .push_constants = { },
                .resource_layouts = ice::Span<ice::render::ResourceSetLayout>{ &_nomaterial_layout, 1 }
            }
        );

        ice::render::ShaderInputAttribute attribs[]{
            ShaderInputAttribute{.location = 0, .offset = 0, .type = ShaderAttribType::Vec2f },
            ShaderInputAttribute{.location = 1, .offset = 8, .type = ShaderAttribType::Vec2f },
        };

        ice::render::ShaderInputBinding shader_bindings[]{
            ShaderInputBinding{
                .binding = 0,
                .stride = 16,
                .instanced = 0,
                .attributes = attribs
            }
        };

        ice::Asset blue_vert = asset_system.request(ice::AssetType::Shader, "/shaders/color/blue-vert"_sid);
        ice::Asset blue_frag = asset_system.request(ice::AssetType::Shader, "/shaders/color/blue-frag"_sid);

        ice::Data blue_vert_data;
        ice::Data blue_frag_data;
        ice::asset_data(blue_vert, blue_vert_data);
        ice::asset_data(blue_frag, blue_frag_data);

        ice::render::ShaderInfo shader_infos[]{
            ShaderInfo{
                .shader_data = *reinterpret_cast<ice::Data const*>(blue_vert_data.location),
                .shader_stage = ice::render::ShaderStageFlags::VertexStage
            },
            ShaderInfo{
                .shader_data = *reinterpret_cast<ice::Data const*>(blue_frag_data.location),
                .shader_stage = ice::render::ShaderStageFlags::FragmentStage
            },
        };
        ice::render::ShaderStageFlags stages[2]{
            ShaderStageFlags::VertexStage,
            ShaderStageFlags::FragmentStage
        };

        _shader_count = ice::size(shader_infos);

        for (ice::u32 idx = 0; idx < _shader_count; ++idx)
        {
            _shaders[idx] = _render_device.create_shader(shader_infos[idx]);
        }

        ice::render::PipelineInfo pipeline_info{
            .layout = _opaque_layout,
            .renderpass = _render_pass,
            .shaders = _shaders,
            .shaders_stages = stages,
            .shader_bindings = shader_bindings
        };

        _opaque_pipeline = _render_device.create_pipeline(
            pipeline_info
        );

        _vertex_buffer = _render_device.create_buffer(
            ice::render::BufferType::Vertex, sizeof(ice::vec3f) * 256
        );

        _indice_buffer = _render_device.create_buffer(
            ice::render::BufferType::Index, sizeof(ice::u16) * 1024
        );

        ice::u16 indices[] = {
            0, 1, 2,
            0, 2, 3,
        };

        ice::vec2f vertices[] = {
            ice::vec2f{ 0.f, 0.f },
            ice::vec2f{ 0.f, 0.f },
            ice::vec2f{ 0.f, 1.f },
            ice::vec2f{ 0.f, 0.f },
            ice::vec2f{ 1.f, 1.f },
            ice::vec2f{ 0.f, 0.f },
            ice::vec2f{ 1.f, 0.f },
            ice::vec2f{ 0.f, 0.f },
        };

        ice::render::BufferUpdateInfo update_info[]{
            BufferUpdateInfo{.buffer = _indice_buffer, .data = ice::data_view(indices, sizeof(indices)) },
            BufferUpdateInfo{.buffer = _vertex_buffer, .data = ice::data_view(vertices, sizeof(vertices)) },
        };

        _render_device.update_buffers(
            update_info
        );
    }

    ~ClearStage() noexcept
    {
        _render_device.destroy_buffer(_indice_buffer);
        _render_device.destroy_buffer(_vertex_buffer);
        _render_device.destroy_pipeline(_opaque_pipeline);

        for (ice::u32 idx = 0; idx < _shader_count; ++idx)
        {
            _render_device.destroy_shader(_shaders[idx]);
        }
        _render_device.destroy_pipeline_layout(_opaque_layout);
        _render_device.destroy_resourcesets(
            ice::Span<ice::render::ResourceSet const>{ &_empty_material, 1 }
        );
        _render_device.destroy_resourceset_layout(_nomaterial_layout);

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
        cmds.bind_pipeline(cmd_buffer, _opaque_pipeline);
        cmds.bind_resource_set(cmd_buffer, _opaque_layout, _empty_material, 0);
        cmds.set_viewport(cmd_buffer, ice::vec4u{ 0, 0, 600, 600 });
        cmds.set_scissor(cmd_buffer, ice::vec4u{ 0, 0, 600, 600 });
        cmds.bind_index_buffer(cmd_buffer, _indice_buffer);
        cmds.bind_vertex_buffer(cmd_buffer, _vertex_buffer, 0);
        cmds.draw_indexed(cmd_buffer, 6, 1);
    }

private:
    ice::render::RenderDevice& _render_device;
    ice::render::RenderSwapchain const& _render_swapchain;
    ice::vec2u _extent;

    ice::render::RenderPass _render_pass;
    ice::render::Image _depth_stencil_image;
    ice::render::Framebuffer _framebuffers[2];

    ice::u32 _shader_count = 0;
    ice::render::ResourceSetLayout _nomaterial_layout;
    ice::render::ResourceSet _empty_material;
    ice::render::Shader _shaders[20]{ };
    ice::render::PipelineLayout _opaque_layout;
    ice::render::Pipeline _opaque_pipeline;
    ice::render::Buffer _indice_buffer;
    ice::render::Buffer _vertex_buffer;
};

class DrawStage final : ice::gfx::GfxStage
{
public:
    void record_commands(
        ice::render::CommandBuffer cmd_buffer,
        ice::render::RenderCommands& cmds
    ) noexcept override
    {
        ice::vec3f points[]{
            ice::vec3f{ 0.0, 0.0, 0.0 },
            ice::vec3f{ 0.5, 0.0, 0.0 },
            ice::vec3f{ 0.0, 0.5, 0.0 },
        };
    }
};

class EndStage final : ice::gfx::GfxStage
{
public:
    void record_commands(
        ice::render::CommandBuffer cmd_buffer,
        ice::render::RenderCommands& cmds
    ) noexcept override
    {
        cmds.end_renderpass(cmd_buffer);
        cmds.end(cmd_buffer);
    }
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
            _engine.asset_system(),
            _gfx_device.swapchain(),
            _render_device
        )
    );

    ice::pod::array::push_back(
        _stages,
        _allocator.make<DrawStage>()
    );

    ice::pod::array::push_back(
        _stages,
        _allocator.make<EndStage>()
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
            ice::stringid_invalid,
            stage,
            { }
        );
    }
}
