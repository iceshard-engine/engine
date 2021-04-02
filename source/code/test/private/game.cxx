#include "game.hxx"
#include <ice/render/render_device.hxx>
#include <ice/render/render_pass.hxx>
#include <ice/render/render_swapchain.hxx>
#include <ice/render/render_resource.hxx>
#include <ice/render/render_buffer.hxx>
#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_frame.hxx>
#include <ice/gfx/gfx_queue.hxx>
#include <ice/gfx/gfx_stage.hxx>
#include <ice/gfx/gfx_resource_tracker.hxx>
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

struct ViewProjectionClip
{
    ice::mat4x4 view;
    ice::mat4x4 projection;
    ice::mat4x4 clip;
};

class ClearStage final : ice::gfx::GfxStage
{
public:
    ClearStage(
        ice::AssetSystem& asset_system,
        ice::gfx::GfxResourceTracker& res_tracker,
        ice::render::RenderSwapchain const& swapchain,
        ice::render::RenderDevice& render_device
    ) noexcept
        : _render_device{ render_device }
        , _resource_tracker{ res_tracker }
        , _render_swapchain{ swapchain }
        , _extent{ swapchain.extent() }
    {
        using namespace ice::render;
        using ice::gfx::GfxResource;

        ResourceSetLayout gui_res_lay = _resource_tracker.find_resource(
            "resourceset_layout.game"_sid, GfxResource::Type::ResourceSetLayout
        ).value.resourceset_layout;

        PipelineLayout gui_lay = _resource_tracker.find_resource(
            "pipeline_layout.game"_sid, GfxResource::Type::PipelineLayout
        ).value.pipeline_layout;

        _render_device.create_resourcesets(
            ice::Span<ice::render::ResourceSetLayout const>{ &gui_res_lay, 1 },
            ice::Span<ice::render::ResourceSet>{ &_empty_material, 1 }
        );

        res_tracker.track_resource(
            "resource_set.empty_material"_sid,
            GfxResource{
                .type = GfxResource::Type::ResourceSet,
                .value = {.resourceset = _empty_material}
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
            },
            ShaderInfo{
                .shader_data = *reinterpret_cast<ice::Data const*>(blue_frag_data.location),
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
            .layout = gui_lay,
            .renderpass = res_tracker.find_resource("renderpass.default"_sid, ice::gfx::GfxResource::Type::Renderpass).value.renderpass,
            .shaders = _shaders,
            .shaders_stages = stages,
            .shader_bindings = shader_bindings,
            .subpass_index = 1,
        };

        _opaque_pipeline = _render_device.create_pipeline(
            pipeline_info
        );

        _camera_data = _render_device.create_buffer(
            ice::render::BufferType::Uniform, sizeof(ViewProjectionClip)
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
            ice::vec2f{ -0.5f, -0.5f },
            ice::vec2f{ 0.f, 0.f },
            ice::vec2f{ 0.5f, -0.5f },
            ice::vec2f{ 0.f, 0.f },
            ice::vec2f{ 0.5f, 0.5f },
            ice::vec2f{ 0.f, 0.f },
            ice::vec2f{ -0.5f, 0.5f },
            ice::vec2f{ 0.f, 0.f },
        };

        ViewProjectionClip vpc{
            .clip = ice::mat4x4{
                .v {
                    { 1.0f, 0.0f, 0.0f, 0.0f },
                    { 0.0f, -1.0f, 0.0f, 0.0f },
                    { 0.0f, 0.0f, 0.5f, 0.5f },
                    { 0.0f, 0.0f, 0.0f, 1.0f },
                }
            }
        };

        ice::render::BufferUpdateInfo update_info[]{
            BufferUpdateInfo{.buffer = _indice_buffer, .data = ice::data_view(indices, sizeof(indices)) },
            BufferUpdateInfo{.buffer = _vertex_buffer, .data = ice::data_view(vertices, sizeof(vertices)) },
            BufferUpdateInfo{.buffer = _camera_data, .data = ice::data_view(vpc) }
        };

        _render_device.update_buffers(
            update_info
        );

        ice::render::ResourceUpdateInfo resources[1]{
            ResourceUpdateInfo {
                .uniform_buffer = {
                    .buffer = _camera_data,
                    .offset = 0,
                    .size = sizeof(ViewProjectionClip)
                }
            },
        };

        ice::render::ResourceSetUpdateInfo camera_update[1]{
            ResourceSetUpdateInfo{
                .resource_set = _empty_material,
                .resource_type = ResourceType::UniformBuffer,
                .binding_index = 0,
                .array_element = 0,
                .resources = { resources + 0, 1 }
            },
        };

        _render_device.update_resourceset(
            camera_update
        );
    }

    ~ClearStage() noexcept
    {
        _render_device.destroy_buffer(_indice_buffer);
        _render_device.destroy_buffer(_vertex_buffer);
        _render_device.destroy_buffer(_camera_data);
        _render_device.destroy_pipeline(_opaque_pipeline);

        for (ice::u32 idx = 0; idx < _shader_count; ++idx)
        {
            _render_device.destroy_shader(_shaders[idx]);
        }
        _render_device.destroy_resourcesets(
            ice::Span<ice::render::ResourceSet const>{ &_empty_material, 1 }
        );
    }

    void record_commands(
        ice::render::CommandBuffer cmd_buffer,
        ice::render::RenderCommands& cmds
    ) noexcept override
    {
        using ice::gfx::GfxResource;
        static ice::vec4f const clear_color{ 0.2f };

        ice::u32 const framebuffer_idx = _render_swapchain.current_image_index();

        ice::StringID constexpr framebuffers[]{
            "ice.gfx.framebuffer[0]"_sid,
            "ice.gfx.framebuffer[1]"_sid
        };

        ice::gfx::GfxResource renderpass = _resource_tracker.find_resource("renderpass.default"_sid, GfxResource::Type::Renderpass);
        ice::gfx::GfxResource framebuffer = _resource_tracker.find_resource(framebuffers[framebuffer_idx], GfxResource::Type::Framebuffer);

        ice::gfx::GfxResource gui_lay = _resource_tracker.find_resource(
            "pipeline_layout.game"_sid, GfxResource::Type::PipelineLayout
        );

        ice::vec4f clear_values[]{
            ice::vec4f{ 0.2f },
            ice::vec4f{ 0.2f },
            ice::vec4f{ 1.0f, 0.0f, 0.0f, 0.0f }
        };

        cmds.begin(cmd_buffer);
        cmds.begin_renderpass(
            cmd_buffer,
            renderpass.value.renderpass,
            framebuffer.value.framebuffer,
            clear_values,
            _extent
        );

        cmds.next_subpass(cmd_buffer, ice::render::SubPassContents::Inline);
        cmds.bind_pipeline(cmd_buffer, _opaque_pipeline);
        cmds.bind_resource_set(cmd_buffer, gui_lay.value.pipeline_layout, _empty_material, 0);
        cmds.set_viewport(cmd_buffer, ice::vec4u{ 0, 0, _extent.x, _extent.y });
        cmds.set_scissor(cmd_buffer, ice::vec4u{ 0, 0, _extent.x, _extent.y });
        cmds.bind_index_buffer(cmd_buffer, _indice_buffer);
        cmds.bind_vertex_buffer(cmd_buffer, _vertex_buffer, 0);
        cmds.draw_indexed(cmd_buffer, 6, 1);
    }

private:
    ice::render::RenderDevice& _render_device;
    ice::render::RenderSwapchain const& _render_swapchain;
    ice::vec2u _extent;

    ice::gfx::GfxResourceTracker& _resource_tracker;

    ice::u32 _shader_count = 0;
    ice::render::ResourceSet _empty_material;
    ice::render::Shader _shaders[20]{ };
    ice::render::Pipeline _opaque_pipeline;
    ice::render::Buffer _indice_buffer;
    ice::render::Buffer _vertex_buffer;
    ice::render::Buffer _camera_data;
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
    EndStage(
        ice::AssetSystem& asset_system,
        ice::gfx::GfxResourceTracker& res_tracker,
        ice::render::RenderSwapchain const& swapchain,
        ice::render::RenderDevice& render_device
    ) noexcept
        : _resource_tracker{ res_tracker }
        , _render_device{ render_device }
        , _swapchain{ swapchain }
    {
        using namespace ice::render;
        using namespace ice::gfx;

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

        ice::Asset pp_vert = asset_system.request(ice::AssetType::Shader, "/shaders/debug/pp-vert"_sid);
        ice::Asset pp_frag = asset_system.request(ice::AssetType::Shader, "/shaders/debug/pp-frag"_sid);

        ice::Data pp_vert_data;
        ice::Data pp_frag_data;
        ice::asset_data(pp_vert, pp_vert_data);
        ice::asset_data(pp_frag, pp_frag_data);

        ice::render::ShaderInfo shader_infos[_shader_count]{
            ShaderInfo{
                .shader_data = *reinterpret_cast<ice::Data const*>(pp_vert_data.location),
            },
            ShaderInfo{
                .shader_data = *reinterpret_cast<ice::Data const*>(pp_frag_data.location),
            },
        };
        ice::render::ShaderStageFlags stages[_shader_count]{
            ShaderStageFlags::VertexStage,
            ShaderStageFlags::FragmentStage
        };

        for (ice::u32 idx = 0; idx < _shader_count; ++idx)
        {
            _shaders[idx] = _render_device.create_shader(shader_infos[idx]);
        }

        PipelineLayout gui_lay = res_tracker.find_resource(
            "pipeline_layout.pp"_sid, GfxResource::Type::PipelineLayout
        ).value.pipeline_layout;


        ice::render::PipelineInfo pipeline_info{
            .layout = gui_lay,
            .renderpass = res_tracker.find_resource("renderpass.default"_sid, ice::gfx::GfxResource::Type::Renderpass).value.renderpass,
            .shaders = _shaders,
            .shaders_stages = stages,
            .shader_bindings = shader_bindings,
            .subpass_index = 2,
        };

        _pp_pipeline = _render_device.create_pipeline(
            pipeline_info
        );

        _vertex_buffer = _render_device.create_buffer(
            ice::render::BufferType::Vertex, sizeof(ice::vec3f) * 256
        );

        _indice_buffer = _render_device.create_buffer(
            ice::render::BufferType::Index, sizeof(ice::u16) * 1024
        );

        static ice::u16 indices[] = {
            0, 1, 2,
        };

        static ice::vec2f vertices[] = {
            ice::vec2f{ -1.0f, -1.0f },
            ice::vec2f{ 0.f, 0.f },
            ice::vec2f{ 3.0f, -1.0f },
            ice::vec2f{ 2.f, 0.f },
            ice::vec2f{ -1.0f, 3.0f },
            ice::vec2f{ 0.f, 2.f },
        };

        ice::render::BufferUpdateInfo update_info[]{
            BufferUpdateInfo{.buffer = _indice_buffer, .data = ice::data_view(indices, sizeof(indices)) },
            BufferUpdateInfo{.buffer = _vertex_buffer, .data = ice::data_view(vertices, sizeof(vertices)) },
        };

        _render_device.update_buffers(
            update_info
        );

        ice::render::ResourceSetLayout pp_layout = _resource_tracker.find_resource("resourceset_layout.pp"_sid, GfxResource::Type::ResourceSetLayout).value.resourceset_layout;

        _render_device.create_resourcesets(
            { &pp_layout, 1 },
            { &_resourceset, 1 }
        );

        ice::render::ResourceUpdateInfo resources[2]{
            ResourceUpdateInfo {
                .image = _resource_tracker.find_resource("image.color_attach"_sid, GfxResource::Type::Image).value.image
            },
            ResourceUpdateInfo {
                .sampler = _resource_tracker.find_resource("sampler.default"_sid, GfxResource::Type::Sampler).value.sampler
            },
        };

        ice::render::ResourceSetUpdateInfo resource_updates[2]{
            ResourceSetUpdateInfo{
                .resource_set = _resourceset,
                .resource_type = ResourceType::InputAttachment,
                .binding_index = 1,
                .array_element = 0,
                .resources = { resources + 0, 1 }
            },
            ResourceSetUpdateInfo{
                .resource_set = _resourceset,
                .resource_type = ResourceType::Sampler,
                .binding_index = 2,
                .array_element = 0,
                .resources = { resources + 1, 1 }
            },
        };

        _render_device.update_resourceset(
            resource_updates
        );
    }

    ~EndStage() noexcept
    {
        _render_device.destroy_resourcesets(
            ice::Span<ice::render::ResourceSet const>{ &_resourceset, 1 }
        );

        _render_device.destroy_shader(_shaders[0]);
        _render_device.destroy_shader(_shaders[1]);

        _render_device.destroy_buffer(_vertex_buffer);
        _render_device.destroy_buffer(_indice_buffer);
        _render_device.destroy_pipeline(_pp_pipeline);
    }

    void record_commands(
        ice::render::CommandBuffer cmd_buffer,
        ice::render::RenderCommands& cmds
    ) noexcept override
    {
        using namespace ice::render;
        using namespace ice::gfx;

        ice::gfx::GfxResource gui_lay = _resource_tracker.find_resource(
            "pipeline_layout.pp"_sid, GfxResource::Type::PipelineLayout
        );

        cmds.next_subpass(cmd_buffer, ice::render::SubPassContents::Inline);
        cmds.bind_pipeline(cmd_buffer, _pp_pipeline);
        cmds.bind_resource_set(cmd_buffer, gui_lay.value.pipeline_layout, _resourceset, 0);
        cmds.bind_vertex_buffer(cmd_buffer, _vertex_buffer, 0);
        cmds.bind_index_buffer(cmd_buffer, _indice_buffer);
        cmds.draw_indexed(cmd_buffer, 3, 1);
        cmds.end_renderpass(cmd_buffer);
        cmds.end(cmd_buffer);
    }

private:
    static ice::u32 constexpr _shader_count = 2;

    ice::gfx::GfxResourceTracker& _resource_tracker;

    ice::render::RenderDevice& _render_device;
    ice::render::RenderSwapchain const& _swapchain;
    ice::render::ResourceSet _resourceset;
    ice::render::Shader _shaders[2];

    ice::render::Pipeline _pp_pipeline;
    ice::render::Buffer _vertex_buffer;
    ice::render::Buffer _indice_buffer;
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
    , _gfx_pass{ _gfx_device.create_pass() }
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

    ice::gfx::GfxResourceTracker& res_tracker = _gfx_device.resource_tracker();

    using namespace ice::render;

    RenderSwapchain const& swapchain = _gfx_device.swapchain();

    RenderAttachment attachments[]{
        RenderAttachment{
            .format = swapchain.image_format(),
            .final_layout = ImageLayout::ShaderReadOnly,
            .type = AttachmentType::TextureImage,
            .operations = {
                AttachmentOperation::Load_Clear,
                AttachmentOperation::Store_DontCare,
            },
        },
        RenderAttachment{
            .format = swapchain.image_format(),
            .final_layout = ImageLayout::Present,
            .type = AttachmentType::SwapchainImage,
            .operations = {
                AttachmentOperation::Load_DontCare,
                AttachmentOperation::Store_Store
            },
        },
        RenderAttachment{
            .format = ImageFormat::SFLOAT_D32_UINT_S8,
            .final_layout = ImageLayout::DepthStencil,
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
            .attachment_index = 0,
            .layout = ImageLayout::ShaderReadOnly
        },
        AttachmentReference{
            .attachment_index = 1,
            .layout = ImageLayout::Color
        },
        AttachmentReference{
            .attachment_index = 2,
            .layout = ImageLayout::DepthStencil
        },
    };

    RenderSubPass subpasses[]{
        RenderSubPass{
            .color_attachments = { references + 0, 1 },
        },
        RenderSubPass{
            .color_attachments = { references + 0, 1 },
            .depth_stencil_attachment = references[3],
        },
        RenderSubPass{
            .input_attachments = { references + 1, 1 },
            .color_attachments = { references + 2, 1 },
        },
    };

    SubpassDependency dependencies[]{
        SubpassDependency{
            .source_subpass = 0,
            .source_stage = PipelineStage::ColorAttachmentOutput,
            .source_access = AccessFlags::ColorAttachmentWrite,
            .destination_subpass = 1,
            .destination_stage = PipelineStage::ColorAttachmentOutput,
            .destination_access = AccessFlags::ColorAttachmentWrite,
        },
        SubpassDependency{
            .source_subpass = 1,
            .source_stage = PipelineStage::ColorAttachmentOutput,
            .source_access = AccessFlags::ColorAttachmentWrite,
            .destination_subpass = 2,
            .destination_stage = PipelineStage::FramentShader,
            .destination_access = AccessFlags::InputAttachmentRead,
        }
    };

    RenderpassInfo renderpass_info{
        .attachments = attachments,
        .subpasses = subpasses,
        .dependencies = dependencies,
    };

    Renderpass renderpass = _render_device.create_renderpass(renderpass_info);
    res_tracker.track_resource(
        "renderpass.default"_sid,
        renderpass
    );

    ice::vec2u extent = swapchain.extent();

    ImageInfo image_info;
    image_info.type = ImageType::Image2D;
    image_info.format = ImageFormat::SFLOAT_D32_UINT_S8;
    image_info.usage = ImageUsageFlags::DepthStencilAttachment;
    image_info.width = extent.x;
    image_info.height = extent.y;

    Image depth_stencil = _render_device.create_image(image_info, { });
    res_tracker.track_resource("image.depth_stencil"_sid, depth_stencil);

    image_info.type = ImageType::Image2D;
    image_info.format = swapchain.image_format();
    image_info.usage = ImageUsageFlags::ColorAttachment | ImageUsageFlags::InputAttachment | ImageUsageFlags::Sampled;
    image_info.width = extent.x;
    image_info.height = extent.y;

    Image color_attach = _render_device.create_image(image_info, { });
    res_tracker.track_resource("image.color_attach"_sid, color_attach);

    Image framebuffer_images[]{
        color_attach,
        Image::Invalid,
        depth_stencil
    };

    SamplerInfo sampler_info{
        .min_filter = SamplerFilter::Linear,
        .mag_filter = SamplerFilter::Linear,
        .address_mode = {
            .u = SamplerAddressMode::Repeat,
            .v = SamplerAddressMode::Repeat,
            .w = SamplerAddressMode::Repeat,
        },
        .mip_map_mode = SamplerMipMapMode::Linear,
    };

    Sampler basic_sampler = _render_device.create_sampler(sampler_info);
    res_tracker.track_resource(
        "sampler.default"_sid,
        ice::gfx::GfxResource{
            .type = ice::gfx::GfxResource::Type::Sampler,
            .value{.sampler = basic_sampler}
        }
    );

    framebuffer_images[1] = swapchain.image(0);
    res_tracker.track_resource(
        "ice.gfx.framebuffer[0]"_sid,
        _render_device.create_framebuffer(
            extent,
            renderpass,
            framebuffer_images
        )
    );

    framebuffer_images[1] = swapchain.image(1);
    res_tracker.track_resource(
        "ice.gfx.framebuffer[1]"_sid,
        _render_device.create_framebuffer(
            extent,
            renderpass,
            framebuffer_images
        )
    );


    ResourceSetLayoutBinding bindings[]{
        ResourceSetLayoutBinding{
            .binding_index = 0,
            .resource_count = 1,
            .resource_type = ResourceType::UniformBuffer,
            .shader_stage_flags = ShaderStageFlags::FragmentStage | ShaderStageFlags::VertexStage
        },
        ResourceSetLayoutBinding {
            .binding_index = 1,
            .resource_count = 1,
            .resource_type = ResourceType::InputAttachment,
            .shader_stage_flags = ShaderStageFlags::FragmentStage
        },
        ResourceSetLayoutBinding {
            .binding_index = 2,
            .resource_count = 1,
            .resource_type = ResourceType::SamplerImmutable,
            .shader_stage_flags = ShaderStageFlags::FragmentStage
        },
    };

    using ice::gfx::GfxResource;

    ResourceSetLayout game_resource_layoyt = _render_device.create_resourceset_layout({ bindings, 1 });
    ResourceSetLayout pp_resource_layout = _render_device.create_resourceset_layout(bindings);

    res_tracker.track_resource(
        "resourceset_layout.game"_sid,
        GfxResource{
            .type = GfxResource::Type::ResourceSetLayout,
            .value = {
                .resourceset_layout = game_resource_layoyt
            }
        }
    );
    res_tracker.track_resource(
        "resourceset_layout.pp"_sid,
        GfxResource{
            .type = GfxResource::Type::ResourceSetLayout,
            .value = {
                .resourceset_layout = pp_resource_layout
            }
        }
    );

    res_tracker.track_resource(
        "pipeline_layout.game"_sid,
        GfxResource{
            .type = GfxResource::Type::PipelineLayout,
            .value = {
                .pipeline_layout = _render_device.create_pipeline_layout(
                    PipelineLayoutInfo{
                        .push_constants = { },
                        .resource_layouts = ice::Span<ice::render::ResourceSetLayout>{ &game_resource_layoyt, 1 }
                    }
                )
            }
        }
    );

    res_tracker.track_resource(
        "pipeline_layout.pp"_sid,
        GfxResource{
            .type = GfxResource::Type::PipelineLayout,
            .value = {
                .pipeline_layout = _render_device.create_pipeline_layout(
                    PipelineLayoutInfo{
                        .push_constants = { },
                        .resource_layouts = ice::Span<ice::render::ResourceSetLayout>{ &pp_resource_layout, 1 }
                    }
                )
            }
        }
    );


    ice::pod::array::push_back(
        _stages,
        _allocator.make<ClearStage>(
            _engine.asset_system(),
            _gfx_device.resource_tracker(),
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
        _allocator.make<EndStage>(
            _engine.asset_system(),
            _gfx_device.resource_tracker(),
            _gfx_device.swapchain(),
            _render_device
        )
    );

}

TestGame::~TestGame() noexcept
{
    ice::gfx::GfxResourceTracker& res_tracker = _gfx_device.resource_tracker();

    using ice::gfx::GfxResource;

    for (ice::gfx::GfxStage* stage : _stages)
    {
        _allocator.destroy(stage);
    }

    _render_device.destroy_framebuffer(
        res_tracker.find_resource("ice.gfx.framebuffer[1]"_sid, GfxResource::Type::Framebuffer).value.framebuffer
    );
    _render_device.destroy_framebuffer(
        res_tracker.find_resource("ice.gfx.framebuffer[0]"_sid, GfxResource::Type::Framebuffer).value.framebuffer
    );
    _render_device.destroy_image(
        res_tracker.find_resource("image.depth_stencil"_sid, GfxResource::Type::Image).value.image
    );
    _render_device.destroy_image(
        res_tracker.find_resource("image.color_attach"_sid, GfxResource::Type::Image).value.image
    );
    _render_device.destroy_sampler(
        res_tracker.find_resource("sampler.default"_sid, GfxResource::Type::Sampler).value.sampler
    );
    _render_device.destroy_renderpass(
        res_tracker.find_resource("renderpass.default"_sid, GfxResource::Type::Renderpass).value.renderpass
    );

    _render_device.destroy_resourceset_layout(
        res_tracker.find_resource("resourceset_layout.game"_sid, GfxResource::Type::ResourceSetLayout).value.resourceset_layout
    );
    _render_device.destroy_resourceset_layout(
        res_tracker.find_resource("resourceset_layout.pp"_sid, GfxResource::Type::ResourceSetLayout).value.resourceset_layout
    );
    _render_device.destroy_pipeline_layout(
        res_tracker.find_resource("pipeline_layout.game"_sid, GfxResource::Type::PipelineLayout).value.pipeline_layout
    );
    _render_device.destroy_pipeline_layout(
        res_tracker.find_resource("pipeline_layout.pp"_sid, GfxResource::Type::PipelineLayout).value.pipeline_layout
    );

    _engine.world_manager().destroy_world("default"_sid);

    _entity_storage = nullptr;
    _archetype_alloc = nullptr;
    _archetype_index = nullptr;

    _gfx_pass = nullptr;
}

void TestGame::update() noexcept
{
    _runner->next_frame();

    ice::EngineFrame& frame = _runner->current_frame();
    ice::gfx::GfxFrame& gfx_frame = _runner->graphics_frame();
    ice::gfx::GfxQueue* default_queue = gfx_frame.get_queue("default"_sid);
    ICE_ASSERT(default_queue != nullptr, "The 'default' graphics pass, does not exist!");

    for (ice::gfx::GfxStage* stage : _stages)
    {
        _gfx_pass->add_stage(stage);
    }

    default_queue->execute_pass(_gfx_pass.get());
}
