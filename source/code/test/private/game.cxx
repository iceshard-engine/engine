#include "game.hxx"
#include <ice/input/input_tracker.hxx>
#include <ice/render/render_device.hxx>
#include <ice/render/render_pass.hxx>
#include <ice/render/render_swapchain.hxx>
#include <ice/render/render_resource.hxx>
#include <ice/render/render_buffer.hxx>
#include <ice/gfx/gfx_model.hxx>
#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_frame.hxx>
#include <ice/gfx/gfx_queue.hxx>
#include <ice/gfx/gfx_stage.hxx>
#include <ice/gfx/gfx_resource_tracker.hxx>
#include <ice/gfx/gfx_subpass.hxx>
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

class NamedStage : public ice::gfx::GfxStage
{
public:
    NamedStage(ice::StringID_Arg name, ice::Span<ice::StringID const> dependencies = { }) noexcept
        : _name{ name }
        , _dependencies{ dependencies }
    { }

    auto name() const noexcept -> ice::StringID
    {
        return _name;
    }

    auto dependencies() const noexcept -> ice::Span<ice::StringID const>
    {
        return _dependencies;
    }

private:
    ice::StringID _name;
    ice::Span<ice::StringID const> _dependencies;
};

class ClearStage final : NamedStage
{
public:
    ClearStage(
        ice::AssetSystem& asset_system,
        ice::gfx::GfxResourceTracker& res_tracker,
        ice::render::RenderSwapchain const& swapchain,
        ice::render::RenderDevice& render_device
    ) noexcept
        : NamedStage{ "test.stage.clear"_sid }
        , _resource_tracker{ res_tracker }
        , _render_swapchain{ swapchain }
        , _extent{ swapchain.extent() }
    {
    }

    ~ClearStage() noexcept = default;

    void record_commands(
        ice::EngineFrame const& frame,
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

        ice::vec4f clear_values[]{
            ice::vec4f{ 0.2f },
            ice::vec4f{ 0.2f },
            ice::vec4f{ 1.0f, 0.0f, 0.0f, 0.0f }
        };

        cmds.begin(cmd_buffer);
        cmds.begin_renderpass(
            cmd_buffer,
            ice::gfx::find_resource<ice::render::Renderpass>(_resource_tracker, "renderpass.default"_sid),
            ice::gfx::find_resource<ice::render::Framebuffer>(_resource_tracker, framebuffers[framebuffer_idx]),
            clear_values,
            _extent
        );

        cmds.next_subpass(cmd_buffer, ice::render::SubPassContents::Inline);
        cmds.set_viewport(cmd_buffer, ice::vec4u{ 0, 0, _extent.x, _extent.y });
        cmds.set_scissor(cmd_buffer, ice::vec4u{ 0, 0, _extent.x, _extent.y });
    }

private:
    ice::render::RenderSwapchain const& _render_swapchain;
    ice::vec2u _extent;

    ice::gfx::GfxResourceTracker& _resource_tracker;
};


class DrawStage final : NamedStage, public ice::WorldTrait
{
    static constexpr ice::StringID Constant_Dependencies[]{
        "test.stage.clear"_sid,
        "camera.update_view"_sid,
    };

public:
    DrawStage(
        ice::AssetSystem& asset_system,
        ice::gfx::GfxResourceTracker& res_tracker,
        ice::render::RenderDevice& render_device
    ) noexcept
        : NamedStage{ "test.stage.draw"_sid, Constant_Dependencies }
        , _render_device{ render_device }
        , _resource_tracker{ res_tracker }
    {
        using namespace ice::render;
        using ice::gfx::GfxResource;

        ice::render::ShaderInputAttribute attribs[]{
            ShaderInputAttribute{.location = 0, .offset = 0, .type = ShaderAttribType::Vec3f },
            ShaderInputAttribute{.location = 1, .offset = 12, .type = ShaderAttribType::Vec3f },
            ShaderInputAttribute{.location = 2, .offset = 24, .type = ShaderAttribType::Vec2f },
            ShaderInputAttribute{.location = 3, .offset = 0, .type = ShaderAttribType::Vec4f },
            ShaderInputAttribute{.location = 4, .offset = 16, .type = ShaderAttribType::Vec4f },
            ShaderInputAttribute{.location = 5, .offset = 32, .type = ShaderAttribType::Vec4f },
            ShaderInputAttribute{.location = 6, .offset = 48, .type = ShaderAttribType::Vec4f },
        };

        ice::render::ShaderInputBinding shader_bindings[]{
            ShaderInputBinding{
                .binding = 0,
                .stride = 32,
                .instanced = 0,
                .attributes = { attribs + 0, 3 }
            },
            ShaderInputBinding{
                .binding = 1,
                .stride = sizeof(ice::mat4),
                .instanced = 1,
                .attributes = { attribs + 3, 4 }
            }
        };

        ice::Asset box_mesh_asset = asset_system.request(ice::AssetType::Mesh, "/mesh/box/dbox"_sid);

        ice::Data box_mesh_data;
        ice::asset_data(box_mesh_asset, box_mesh_data);

        _model = reinterpret_cast<ice::gfx::Model const*>(box_mesh_data.location);

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
            .layout = ice::gfx::find_resource<PipelineLayout>(res_tracker, ice::gfx::GfxSubpass_Primitives::ResName_PipelineLayout),
            .renderpass = ice::gfx::find_resource<Renderpass>(res_tracker, "renderpass.default"_sid),
            .shaders = _shaders,
            .shaders_stages = stages,
            .shader_bindings = shader_bindings,
            .cull_mode = CullMode::BackFace,
            .front_face = FrontFace::CounterClockWise,
            .subpass_index = 1,
            .depth_test = true
        };

        _opaque_pipeline = _render_device.create_pipeline(
            pipeline_info
        );

        _vertex_buffer[0] = _render_device.create_buffer(
            ice::render::BufferType::Vertex, sizeof(ice::mat4) * 32 * 32
        );

        _indice_buffer[0] = _render_device.create_buffer(
            ice::render::BufferType::Vertex, sizeof(ice::u16) * 1024
        );

        _vertex_buffer[1] = _render_device.create_buffer(
            ice::render::BufferType::Vertex, sizeof(ice::vec3f) * 256
        );

        _indice_buffer[1] = _render_device.create_buffer(
            ice::render::BufferType::Index, sizeof(ice::u16) * 1024
        );

        for (ice::u32 i = 0; i < 32; ++i)
        {
            for (ice::u32 j = 0; j < 32; ++j)
            {
                ice::vec3i rand{ (::rand() % 100) - 50, (::rand() % 100) - 50, (::rand() % 100) - 50 };

                _instances[i * 32 + j] = ice::translate(ice::vec3f(i, j, i + j) + rand);
            }
        }

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

        ice::render::BufferUpdateInfo update_info[]{
            BufferUpdateInfo{.buffer = _indice_buffer[0], .data = ice::data_view(indices, sizeof(indices)) },
            BufferUpdateInfo{.buffer = _indice_buffer[1], .data = ice::data_view(_model->indice_data, _model->indice_data_size) },
            BufferUpdateInfo{.buffer = _vertex_buffer[0], .data = ice::data_view(_instances, sizeof(_instances)) },
            BufferUpdateInfo{.buffer = _vertex_buffer[1], .data = ice::data_view(_model->vertice_data, _model->vertice_data_size) },
        };

        _render_device.update_buffers(
            update_info
        );
    }

    ~DrawStage() noexcept
    {
        _render_device.destroy_buffer(_indice_buffer[0]);
        _render_device.destroy_buffer(_vertex_buffer[0]);
        _render_device.destroy_buffer(_indice_buffer[1]);
        _render_device.destroy_buffer(_vertex_buffer[1]);
        _render_device.destroy_pipeline(_opaque_pipeline);

        for (ice::u32 idx = 0; idx < _shader_count; ++idx)
        {
            _render_device.destroy_shader(_shaders[idx]);
        }
    }

    void on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::World& world
    ) noexcept override
    {

        //for (ice::u32 i = 0; i < 32; ++i)
        //{
        //    for (ice::u32 j = 0; j < 32; ++j)
        //    {
        //        ice::vec3f rand((::rand() % 100) - 50, (::rand() % 100) - 50, (::rand() % 100) - 50);
        //        rand = rand / 10000.f;

        //        _instances[i * 32 + j] = ice::translate(_instances[i * 32 + j], rand);
        //    }
        //}

        //ice::render::BufferUpdateInfo update_info[]{
        //    ice::render::BufferUpdateInfo{.buffer = _vertex_buffer[0], .data = ice::data_view(_instances, sizeof(_instances)) },
        //};

        //_render_device.update_buffers(
        //    update_info
        //);
    }

    void record_commands(
        ice::EngineFrame const& frame,
        ice::render::CommandBuffer cmd_buffer,
        ice::render::RenderCommands& cmds
    ) noexcept override
    {
        ice::vec3f points[]{
            ice::vec3f{ 0.0, 0.0, 0.0 },
            ice::vec3f{ 0.5, 0.0, 0.0 },
            ice::vec3f{ 0.0, 0.5, 0.0 },
        };

        cmds.bind_pipeline(cmd_buffer, _opaque_pipeline);
        //cmds.bind_index_buffer(cmd_buffer, _indice_buffer[0]);
        //cmds.bind_vertex_buffer(cmd_buffer, _vertex_buffer[0], 0);
        //cmds.draw_indexed(cmd_buffer, 6, 1);
        cmds.bind_index_buffer(cmd_buffer, _indice_buffer[1]);
        cmds.bind_vertex_buffer(cmd_buffer, _vertex_buffer[1], 0);
        cmds.bind_vertex_buffer(cmd_buffer, _vertex_buffer[0], 1);
        cmds.draw_indexed(
            cmd_buffer,
            _model->mesh_list[0].indice_count, 32 * 32,
            _model->mesh_list[0].indice_offset,
            _model->mesh_list[0].vertice_offset,
            0
        );
    }

private:
    ice::render::RenderDevice& _render_device;
    ice::gfx::GfxResourceTracker& _resource_tracker;


    ice::mat4 _instances[32 * 32]{};
    ice::gfx::Model const* _model;

    ice::u32 _shader_count = 0;
    ice::render::Shader _shaders[20]{ };
    ice::render::Pipeline _opaque_pipeline;
    ice::render::Buffer _indice_buffer[2];
    ice::render::Buffer _vertex_buffer[2];
};

class EndStage final : NamedStage
{
    static constexpr ice::StringID Constant_Dependencies[]{
        "test.stage.draw"_sid
    };

public:
    EndStage(
        ice::AssetSystem& asset_system,
        ice::gfx::GfxResourceTracker& res_tracker,
        ice::render::RenderSwapchain const& swapchain,
        ice::render::RenderDevice& render_device
    ) noexcept
        : NamedStage{ "test.stage.end"_sid, Constant_Dependencies }
        , _resource_tracker{ res_tracker }
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
        ice::EngineFrame const& frame,
        ice::render::CommandBuffer cmd_buffer,
        ice::render::RenderCommands& cmds
    ) noexcept override
    {
        using namespace ice::render;
        using namespace ice::gfx;

        ice::gfx::GfxResource gui_lay = _resource_tracker.find_resource(
            "pipeline_layout.pp"_sid, GfxResource::Type::PipelineLayout
        );

        auto extent = _swapchain.extent();
        cmds.set_viewport(cmd_buffer, ice::vec4u{ 0, 0, extent.x, extent.y });
        cmds.set_scissor(cmd_buffer, ice::vec4u{ 0, 0, extent.x, extent.y });

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
    , _terrain{ _allocator, _engine }
    , _camera_manager{ _allocator, _engine }
    , _imgui{ _allocator, _engine, _gfx_device.swapchain() }
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
                AttachmentOperation::Load_Clear,
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
    ice::gfx::track_resource(
        res_tracker,
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

    ice::gfx::track_resource(res_tracker, "image.depth_stencil"_sid, depth_stencil);

    image_info.type = ImageType::Image2D;
    image_info.format = swapchain.image_format();
    image_info.usage = ImageUsageFlags::ColorAttachment | ImageUsageFlags::InputAttachment | ImageUsageFlags::Sampled;
    image_info.width = extent.x;
    image_info.height = extent.y;

    Image color_attach = _render_device.create_image(image_info, { });
    ice::gfx::track_resource(res_tracker, "image.color_attach"_sid, color_attach);

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
    ice::gfx::track_resource(res_tracker,
        "ice.gfx.framebuffer[0]"_sid,
        _render_device.create_framebuffer(
            extent,
            renderpass,
            framebuffer_images
        )
    );

    framebuffer_images[1] = swapchain.image(1);
    ice::gfx::track_resource(res_tracker,
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

    ResourceSetLayout pp_resource_layout = _render_device.create_resourceset_layout(bindings);

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

    _world = _engine.world_manager().create_world(
        "default"_sid, _entity_storage.get()
    );

    _world->add_trait("test.terrain"_sid, &_terrain);
    _world->add_trait("test.cameras"_sid, &_camera_manager);
    _world->add_trait("test.imgui"_sid, &_imgui);

    ice::EngineRequest requests[]{
        ice::create_request(ice::Request_ActivateWorld, _world)
    };
    _runner->current_frame().push_requests(requests);


    ice::pod::array::push_back(
        _stages,
        _allocator.make<ClearStage>(
            _engine.asset_system(),
            _gfx_device.resource_tracker(),
            _gfx_device.swapchain(),
            _render_device
        )
    );

    DrawStage* stage = _allocator.make<DrawStage>(
        _engine.asset_system(),
        _gfx_device.resource_tracker(),
        _render_device
    );

    ice::pod::array::push_back(
        _stages,
        stage
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

    _world->add_trait("test.draw"_sid, stage);

}

TestGame::~TestGame() noexcept
{
    ice::gfx::GfxResourceTracker& res_tracker = _gfx_device.resource_tracker();

    using ice::gfx::GfxResource;

    _world->remove_trait("test.draw"_sid);

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
        res_tracker.find_resource("resourceset_layout.pp"_sid, GfxResource::Type::ResourceSetLayout).value.resourceset_layout
    );
    _render_device.destroy_pipeline_layout(
        res_tracker.find_resource("pipeline_layout.pp"_sid, GfxResource::Type::PipelineLayout).value.pipeline_layout
    );


    _entity_storage = nullptr;
    _archetype_alloc = nullptr;
    _archetype_index = nullptr;

    _gfx_pass = nullptr;

    _runner = nullptr;
    _engine.world_manager().destroy_world("default"_sid);
}

void TestGame::update() noexcept
{
    _runner->next_frame();

    _gfx_pass->add_stage(
        _stages[0]->name(),
        _stages[0]->dependencies(),
        _stages[0]
    );

    static ice::StringID deps[]{
        "test.stage.clear"_sid
    };

    _gfx_pass->add_stage(
        "camera.update_view"_sid,
        deps,
        &_camera_manager
    );

    _gfx_pass->add_stage(
        _stages[1]->name(),
        _stages[1]->dependencies(),
        _stages[1]
    );
    static ice::StringID deps2[]{
        "camera.update_view"_sid,
    };

    _gfx_pass->add_stage(
        "imgui"_sid,
        deps2,
        &_imgui
    );

    _gfx_pass->add_stage(
        _stages[2]->name(),
        _stages[2]->dependencies(),
        _stages[2]
    );

    _runner->graphics_frame().enqueue_pass(
        "default"_sid,
        _gfx_pass.get()
    );
}

void TestGame::update_inputs(ice::input::DeviceQueue const& deivce_queue) noexcept
{
    _runner->process_device_queue(deivce_queue);
}
