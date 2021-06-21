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
#include <ice/world/world_portal.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_frame.hxx>

#include <ice/asset.hxx>
#include <ice/asset_system.hxx>
#include <ice/stack_string.hxx>

#include <ice/task.hxx>
#include <ice/assert.hxx>
#include <ice/log.hxx>
#include <ice/stringid.hxx>
#include <ice/uri.hxx>

using ice::operator""_sid;
using ice::operator""_uri;

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

class ClearStage final : public NamedStage, public ice::WorldTrait
{
public:
    ClearStage() noexcept
        : NamedStage{ "test.stage.clear"_sid }
    {
    }

    ~ClearStage() noexcept = default;

    void on_activate(ice::Engine&, ice::EngineRunner& r, ice::WorldPortal& p) noexcept override
    {
        ice::gfx::GfxResourceTracker& res_tracker = r.graphics_device().resource_tracker();

        ice::StringID constexpr framebuffers[]{
            "ice.gfx.framebuffer[0]"_sid,
            "ice.gfx.framebuffer[1]"_sid
        };

        _render_swapchain = &r.graphics_device().swapchain();

        _renderpass = ice::gfx::find_resource<ice::render::Renderpass>(res_tracker, "renderpass.default"_sid);
        _framebuffers[0] = ice::gfx::find_resource<ice::render::Framebuffer>(res_tracker, framebuffers[0]);
        _framebuffers[1] = ice::gfx::find_resource<ice::render::Framebuffer>(res_tracker, framebuffers[1]);
    }

    void on_deactivate(ice::Engine&, ice::EngineRunner& r, ice::WorldPortal& p) noexcept override
    {
        ice::gfx::GfxResourceTracker& res_tracker = r.graphics_device().resource_tracker();
    }

    void record_commands(
        ice::EngineFrame const& frame,
        ice::render::CommandBuffer cmd_buffer,
        ice::render::RenderCommands& cmds
    ) noexcept override
    {
        using ice::gfx::GfxResource;
        static ice::vec4f const clear_color{ 0.2f };

        ice::u32 const framebuffer_idx = _render_swapchain->current_image_index();
        _extent = _render_swapchain->extent();

        ice::vec4f clear_values[]{
            ice::vec4f{ 0.2f },
            ice::vec4f{ 0.2f },
            ice::vec4f{ 1.0f, 0.0f, 0.0f, 0.0f }
        };

        cmds.begin(cmd_buffer);
        cmds.begin_renderpass(
            cmd_buffer,
            _renderpass,
            _framebuffers[framebuffer_idx],
            clear_values,
            _extent
        );

        cmds.next_subpass(cmd_buffer, ice::render::SubPassContents::Inline);
        cmds.set_viewport(cmd_buffer, ice::vec4u{ 0, 0, _extent.x, _extent.y });
        cmds.set_scissor(cmd_buffer, ice::vec4u{ 0, 0, _extent.x, _extent.y });
    }

private:
    ice::render::RenderSwapchain const* _render_swapchain;
    ice::vec2u _extent;

    ice::render::Renderpass _renderpass;
    ice::render::Framebuffer _framebuffers[2];
};


class DrawStage final : public NamedStage, public ice::WorldTrait
{
    static constexpr ice::StringID Constant_Dependencies[]{
        "test.stage.clear"_sid,
        "camera.update_view"_sid,
    };

public:
    DrawStage(
        ice::AssetSystem& asset_system
    ) noexcept
        : NamedStage{ "test.stage.draw"_sid, Constant_Dependencies }
    {
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

        _shader_data[0] = *reinterpret_cast<ice::Data const*>(blue_vert_data.location);
        _shader_data[1] = *reinterpret_cast<ice::Data const*>(blue_frag_data.location);
    }

    void on_activate(
        ice::Engine&,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept override
    {
        ice::gfx::GfxDevice& gfx_device = runner.graphics_device();
        ice::gfx::GfxResourceTracker& res_tracker = gfx_device.resource_tracker();

        ice::render::RenderDevice& render_device = gfx_device.device();

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

        ice::render::ShaderInfo shader_infos[]{
            ShaderInfo{
                .shader_data = _shader_data[0],
            },
            ShaderInfo{
                .shader_data = _shader_data[1],
            },
        };
        ice::render::ShaderStageFlags stages[2]{
            ShaderStageFlags::VertexStage,
            ShaderStageFlags::FragmentStage
        };

        _shader_count = ice::size(shader_infos);

        for (ice::u32 idx = 0; idx < _shader_count; ++idx)
        {
            _shaders[idx] = render_device.create_shader(shader_infos[idx]);
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

        _opaque_pipeline = render_device.create_pipeline(
            pipeline_info
        );

        _vertex_buffer[0] = render_device.create_buffer(
            ice::render::BufferType::Vertex, sizeof(ice::mat4) * 32 * 32
        );

        _indice_buffer[0] = render_device.create_buffer(
            ice::render::BufferType::Vertex, sizeof(ice::u16) * 1024
        );

        _vertex_buffer[1] = render_device.create_buffer(
            ice::render::BufferType::Vertex, sizeof(ice::vec3f) * 256
        );

        _indice_buffer[1] = render_device.create_buffer(
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

        render_device.update_buffers(
            update_info
        );
    }

    void on_deactivate(ice::Engine&, ice::EngineRunner& runner, ice::WorldPortal& portal) noexcept override
    {
        ice::gfx::GfxDevice& gfx_device = runner.graphics_device();
        ice::gfx::GfxResourceTracker& res_tracker = gfx_device.resource_tracker();
        ice::render::RenderDevice& render_device = gfx_device.device();

        render_device.destroy_buffer(_indice_buffer[0]);
        render_device.destroy_buffer(_vertex_buffer[0]);
        render_device.destroy_buffer(_indice_buffer[1]);
        render_device.destroy_buffer(_vertex_buffer[1]);
        render_device.destroy_pipeline(_opaque_pipeline);

        for (ice::u32 idx = 0; idx < _shader_count; ++idx)
        {
            render_device.destroy_shader(_shaders[idx]);
        }
    }

    void on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
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
        //cmds.draw_indexed(
        //    cmd_buffer,
        //    _model->mesh_list[0].indice_count, 32 * 32,
        //    _model->mesh_list[0].indice_offset,
        //    _model->mesh_list[0].vertice_offset,
        //    0
        //);
    }

private:
    ice::mat4 _instances[32 * 32]{};
    ice::gfx::Model const* _model;

    ice::u32 _shader_count = 0;
    ice::Data _shader_data[2];
    ice::render::Shader _shaders[20]{ };
    ice::render::Pipeline _opaque_pipeline;
    ice::render::Buffer _indice_buffer[2];
    ice::render::Buffer _vertex_buffer[2];
};

class EndStage final : public NamedStage, public ice::WorldTrait
{
    static constexpr ice::StringID Constant_Dependencies[]{
        "test.stage.draw"_sid
    };

public:
    EndStage(ice::AssetSystem& asset_system) noexcept
        : NamedStage{ "test.stage.end"_sid, Constant_Dependencies }
    {
        ice::Asset pp_vert = asset_system.request(ice::AssetType::Shader, "/shaders/debug/pp-vert"_sid);
        ice::Asset pp_frag = asset_system.request(ice::AssetType::Shader, "/shaders/debug/pp-frag"_sid);

        ice::Data pp_vert_data;
        ice::Data pp_frag_data;
        ice::asset_data(pp_vert, pp_vert_data);
        ice::asset_data(pp_frag, pp_frag_data);

        _shader_data[0] = *reinterpret_cast<ice::Data const*>(pp_vert_data.location);
        _shader_data[1] = *reinterpret_cast<ice::Data const*>(pp_frag_data.location);
    }

    void on_activate(ice::Engine&, ice::EngineRunner& r, ice::WorldPortal& p) noexcept override
    {
        ice::gfx::GfxResourceTracker& res_tracker = r.graphics_device().resource_tracker();
        ice::render::RenderDevice& render_device = r.graphics_device().device();

        using namespace ice::render;
        using namespace ice::gfx;

        _swapchain = &r.graphics_device().swapchain();

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

        ice::render::ShaderInfo shader_infos[_shader_count]{
            ShaderInfo{
                .shader_data = _shader_data[0],
            },
            ShaderInfo{
                .shader_data = _shader_data[1],
            },
        };
        ice::render::ShaderStageFlags stages[_shader_count]{
            ShaderStageFlags::VertexStage,
            ShaderStageFlags::FragmentStage
        };

        for (ice::u32 idx = 0; idx < _shader_count; ++idx)
        {
            _shaders[idx] = render_device.create_shader(shader_infos[idx]);
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

        _pp_pipeline = render_device.create_pipeline(
            pipeline_info
        );

        _vertex_buffer = render_device.create_buffer(
            ice::render::BufferType::Vertex, sizeof(ice::vec3f) * 256
        );

        _indice_buffer = render_device.create_buffer(
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

        render_device.update_buffers(
            update_info
        );

        ice::render::ResourceSetLayout pp_layout = find_resource<ResourceSetLayout>(res_tracker, "resourceset_layout.pp"_sid);

        render_device.create_resourcesets(
            { &pp_layout, 1 },
            { &_resourceset, 1 }
        );

        ice::render::ResourceUpdateInfo resources[2]{
            ResourceUpdateInfo {
                .image = find_resource<Image>(res_tracker, "image.color_attach"_sid)
            },
            ResourceUpdateInfo {
                .sampler = find_resource<Sampler>(res_tracker, "sampler.default"_sid)
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

        render_device.update_resourceset(
            resource_updates
        );

        _pp_pipeline_layout = find_resource<PipelineLayout>(
            res_tracker,
            "pipeline_layout.pp"_sid
        );
    }

    void on_deactivate(ice::Engine&, ice::EngineRunner& r, ice::WorldPortal& p) noexcept override
    {
        ice::render::RenderDevice& render_device = r.graphics_device().device();

        render_device.destroy_resourcesets(
            ice::Span<ice::render::ResourceSet const>{ &_resourceset, 1 }
        );

        render_device.destroy_shader(_shaders[0]);
        render_device.destroy_shader(_shaders[1]);

        render_device.destroy_buffer(_vertex_buffer);
        render_device.destroy_buffer(_indice_buffer);
        render_device.destroy_pipeline(_pp_pipeline);
    }

    void record_commands(
        ice::EngineFrame const& frame,
        ice::render::CommandBuffer cmd_buffer,
        ice::render::RenderCommands& cmds
    ) noexcept override
    {
        using namespace ice::render;
        using namespace ice::gfx;

        auto extent = _swapchain->extent();
        cmds.set_viewport(cmd_buffer, ice::vec4u{ 0, 0, extent.x, extent.y });
        cmds.set_scissor(cmd_buffer, ice::vec4u{ 0, 0, extent.x, extent.y });

        cmds.next_subpass(cmd_buffer, ice::render::SubPassContents::Inline);
        cmds.bind_pipeline(cmd_buffer, _pp_pipeline);
        cmds.bind_resource_set(cmd_buffer, _pp_pipeline_layout, _resourceset, 0);
        cmds.bind_vertex_buffer(cmd_buffer, _vertex_buffer, 0);
        cmds.bind_index_buffer(cmd_buffer, _indice_buffer);
        cmds.draw_indexed(cmd_buffer, 3, 1);
        cmds.end_renderpass(cmd_buffer);
        cmds.end(cmd_buffer);
    }

private:
    static ice::u32 constexpr _shader_count = 2;

    ice::Data _shader_data[2];
    ice::render::RenderSwapchain const* _swapchain;
    ice::render::ResourceSet _resourceset;
    ice::render::Shader _shaders[2];

    ice::render::PipelineLayout _pp_pipeline_layout;
    ice::render::Pipeline _pp_pipeline;
    ice::render::Buffer _vertex_buffer;
    ice::render::Buffer _indice_buffer;
};

class RenderObjects_Trait : public ice::WorldTrait
{
public:
    void on_activate(
        ice::Engine&,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept override
    {
        ice::gfx::GfxDevice& gfx_device = runner.graphics_device();
        ice::gfx::GfxResourceTracker& res_tracker = gfx_device.resource_tracker();

        ice::render::RenderDevice& render_device = gfx_device.device();

        using namespace ice::render;

        RenderSwapchain const& swapchain = gfx_device.swapchain();

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

        Renderpass renderpass = render_device.create_renderpass(renderpass_info);
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

        Image depth_stencil = render_device.create_image(image_info, { });

        ice::gfx::track_resource(res_tracker, "image.depth_stencil"_sid, depth_stencil);

        image_info.type = ImageType::Image2D;
        image_info.format = swapchain.image_format();
        image_info.usage = ImageUsageFlags::ColorAttachment | ImageUsageFlags::InputAttachment | ImageUsageFlags::Sampled;
        image_info.width = extent.x;
        image_info.height = extent.y;

        Image color_attach = render_device.create_image(image_info, { });
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

        Sampler basic_sampler = render_device.create_sampler(sampler_info);
        ice::gfx::track_resource(res_tracker, "sampler.default"_sid, basic_sampler);

        framebuffer_images[1] = swapchain.image(0);
        ice::gfx::track_resource(res_tracker,
            "ice.gfx.framebuffer[0]"_sid,
            render_device.create_framebuffer(
                extent,
                renderpass,
                framebuffer_images
            )
        );

        framebuffer_images[1] = swapchain.image(1);
        ice::gfx::track_resource(res_tracker,
            "ice.gfx.framebuffer[1]"_sid,
            render_device.create_framebuffer(
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

        ResourceSetLayout pp_resource_layout = render_device.create_resourceset_layout(bindings);

        ice::gfx::track_resource(
            res_tracker,
            "resourceset_layout.pp"_sid,
            pp_resource_layout
        );

        ice::gfx::track_resource(
            res_tracker,
            "pipeline_layout.pp"_sid,
            render_device.create_pipeline_layout(
                PipelineLayoutInfo{
                    .push_constants = { },
                    .resource_layouts = ice::Span<ice::render::ResourceSetLayout>{ &pp_resource_layout, 1 }
                }
            )
        );
    }

    void on_deactivate(
        ice::Engine&,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept override
    {
        ice::gfx::GfxDevice& gfx_device = runner.graphics_device();
        ice::gfx::GfxResourceTracker& res_tracker = gfx_device.resource_tracker();

        ice::render::RenderDevice& render_device = gfx_device.device();

        using namespace ice::gfx;
        using namespace ice::render;

        render_device.destroy_framebuffer(
            find_resource<Framebuffer>(res_tracker, "ice.gfx.framebuffer[1]"_sid)
        );
        render_device.destroy_framebuffer(
            find_resource<Framebuffer>(res_tracker, "ice.gfx.framebuffer[0]"_sid)
        );
        render_device.destroy_image(
            find_resource<Image>(res_tracker, "image.depth_stencil"_sid)
        );
        render_device.destroy_image(
            find_resource<Image>(res_tracker, "image.color_attach"_sid)
        );
        render_device.destroy_sampler(
            find_resource<Sampler>(res_tracker, "sampler.default"_sid)
        );
        render_device.destroy_renderpass(
            find_resource<Renderpass>(res_tracker, "renderpass.default"_sid)
        );

        render_device.destroy_resourceset_layout(
            find_resource<ResourceSetLayout>(res_tracker, "resourceset_layout.pp"_sid)
        );
        render_device.destroy_pipeline_layout(
            find_resource<PipelineLayout>(res_tracker, "pipeline_layout.pp"_sid)
        );
    }

    void on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::World& world
    ) noexcept
    {

    }
};

struct MyGame::TraitContainer : public ice::WorldTrait
{
    TraitContainer(
        ice::Allocator& alloc,
        ice::Engine& engine,
        ice::AssetSystem& asset_system
    ) noexcept
        : _render_objects{ }
        , _camera{ alloc, engine }
        , _terrain{ alloc, engine }
        , _imgui{ alloc, engine }
        , _clear{ }
        , _draw{ asset_system }
        , _end{ asset_system }
    {
    }

    void list_allocator_allocations(ice::StackString<1024>& offset, ice::TrackedAllocator const* ta) noexcept
    {
        while (ta != nullptr)
        {
            ICE_LOG(
                ice::LogSeverity::Debug, ice::LogTag::Game,
                "{}# Allocator `{}` total: {} [{:.3} MiB]",
                offset,
                ta->name(),
                ta->total_allocated(),
                ice::f32(ta->total_allocated()) / (1024.f * 1024.f)
            );
            if (ta->child_allocators())
            {
                ICE_LOG(
                    ice::LogSeverity::Debug, ice::LogTag::Game,
                    "{}> Sub-Allocators...",
                    offset
                );
                ice::string::push_back(offset, "| ");
                list_allocator_allocations(offset, ta->child_allocators());
                ice::string::pop_back(offset, 2);
            }
            ta = ta->next_sibling();
        }
    };

    void on_update(ice::EngineFrame& f, ice::EngineRunner& r, ice::WorldPortal&) noexcept override
    {
        //if constexpr (ice::build::is_debug)
        //{
        //    ice::StackString<1024> offset{ "" };
        //    ice::TrackedAllocator& tracked_alloc = static_cast<ice::TrackedAllocator&>(ice::memory::default_allocator());
        //    list_allocator_allocations(offset, &tracked_alloc);
        //}

        f.execute_task(update_stages(r.graphics_device().aquire_pass("pass.default"_sid), r));
    }

    auto update_stages(ice::gfx::GfxPass& pass, ice::EngineRunner& r) noexcept -> ice::Task<>
    {
        pass.add_stage(
            _clear.name(),
            _clear.dependencies(),
            &_clear
        );

        static ice::StringID deps[]{
            "test.stage.clear"_sid
        };

        static ice::StringID deps2[]{
            "camera.update_view"_sid,
        };

        pass.add_stage(
            "terrain"_sid,
            deps2,
            &_terrain
        );

        pass.add_stage(
            "camera.update_view"_sid,
            deps,
            &_camera
        );

        pass.add_stage(
            _draw.name(),
            _draw.dependencies(),
            &_draw
        );

        //pass.add_stage(
        //    "imgui"_sid,
        //    deps2,
        //    &_imgui
        //);

        pass.add_stage(
            _end.name(),
            _end.dependencies(),
            &_end
        );

        r.graphics_frame().enqueue_pass(
            "default"_sid,
            &pass
        );

        co_return;
    }

    RenderObjects_Trait _render_objects;
    ice::trait::CameraManager _camera;
    ice::trait::Terrain _terrain;
    ice::Ice_ImGui _imgui;

    ClearStage _clear;
    DrawStage _draw;
    EndStage _end;
};

void MyGame::on_game_begin(ice::EngineRunner& runner) noexcept
{
    ice::EngineRequest requests[]{
        ice::create_request(ice::Request_ActivateWorld, _current_engine->world_manager().find_world("default"_sid))
    };
    runner.current_frame().push_requests(requests);
}

void MyGame::on_game_end() noexcept
{
}

void MyGame::add_world_traits(
    ice::Engine& engine,
    ice::World* world
) noexcept
{
    _traits = _allocator.make<TraitContainer>(_allocator, engine, engine.asset_system());
    _game2d_trait = ice::create_game2d_trait(_allocator);
    _tilemap_trait = ice::create_tilemap_trait(_allocator, engine.asset_system());
    _physics_trait = ice::create_physics2d_trait(_allocator, _clock);

    static ice::TileMaterialGroup tilemap_mats[1]
    {
        ice::TileMaterialGroup
        {
            .group_id = 0,
            .image_asset = "/cotm/tileset_a"_sid,
        }
    };

    static ice::vec2f tiles_pos[16 * 16];
    static ice::TileMaterial tiles_mat[16 * 16];

    for (ice::u32 i = 0; i < 16; ++i)
    {
        for (ice::u32 j = 0; j < 16; ++j)
        {
            tiles_pos[i * 16 + j] = { 32.f * j, 32.f * i };

            tiles_mat[i * 16 + j] = ice::TileMaterial{ .group_id = 0, .material_id_x = 1, .material_id_y = 0 };
            if (j == 0 || j == 15)
            {
                tiles_mat[i * 16 + j] = ice::TileMaterial{ .group_id = 0, .material_id_x = 0, .material_id_y = 0 };
            }
            if (i == 0)
            {
                tiles_mat[i * 16 + j] = ice::TileMaterial{ .group_id = 0, .material_id_x = 2, .material_id_y = 0 };
            }
            if (i == 15)
            {
                tiles_mat[i * 16 + j] = ice::TileMaterial{ .group_id = 0, .material_id_x = 0, .material_id_y = 1 };
            }

        }
    }

    //static ice::vec2f tiles_pos[2]
    //{
    //    ice::vec2f{ 0.f, 0.f },
    //    ice::vec2f{ 0.f, 32.f },
    //};

    //static ice::TileMaterial tiles_mat[2]
    //{
    //    ice::TileMaterial{.group_id = 0, .material_id_x = 0, .material_id_y = 0 },
    //    ice::TileMaterial{.group_id = 0, .material_id_x = 1, .material_id_y = 10 },
    //};

    static ice::TileRoom tilemap_rooms[1]
    {
        ice::TileRoom
        {
            .name = "test"_sid,
            .portals = { },
            .tiles_count = ice::size(tiles_pos),
            .tiles_position = tiles_pos,
            .tiles_material = tiles_mat
        }
    };

    static ice::TileMap tilemap
    {
        .name = "test"_sid,
        .material_groups = tilemap_mats,
        .rooms = tilemap_rooms,
    };

    ice::TileMaterial rigid_materials[]{
        ice::TileMaterial{.group_id = 0, .material_id_x = 0, .material_id_y = 1 },
        ice::TileMaterial{.group_id = 0, .material_id_x = 2, .material_id_y = 0 },
    };

    _tilemap_trait->load_tilemap("test"_sid, tilemap);
    _physics_trait->load_tilemap_room(tilemap.rooms[0], rigid_materials);

    world->add_trait("game"_sid, _traits);
    world->add_trait("render.objects"_sid, &_traits->_render_objects);
    world->add_trait("game2d"_sid, _game2d_trait.get());
    world->add_trait("tilemap"_sid, _tilemap_trait.get());
    world->add_trait("tilemap.physics"_sid, _physics_trait.get());

    //world->add_trait("game.update"_sid, _traits->);
    world->add_trait("test.cameras"_sid, &_traits->_camera);
    world->add_trait("test.terrain"_sid, &_traits->_terrain);
    //world->add_trait("test.imgui"_sid, &_traits->_imgui);
    world->add_trait("test.clear"_sid, &_traits->_clear);
    world->add_trait("test.draw"_sid, &_traits->_draw);
    world->add_trait("test.end"_sid, &_traits->_end);
}

void MyGame::remove_world_traits(ice::World* world) noexcept
{
    world->remove_trait("test.end"_sid);
    world->remove_trait("test.draw"_sid);
    world->remove_trait("test.clear"_sid);
    //world->remove_trait("test.imgui"_sid);
    world->remove_trait("test.terrain"_sid);
    world->remove_trait("test.cameras"_sid);
    //world->remove_trait("game.update"_sid);

    world->remove_trait("tilemap.physics"_sid);
    world->remove_trait("tilemap"_sid);
    world->remove_trait("game2d"_sid);
    world->remove_trait("render.objects"_sid);
    world->remove_trait("game"_sid);

    _physics_trait = nullptr;
    _tilemap_trait = nullptr;
    // _game2d_trait = nullptr;
    _allocator.destroy(_traits);
}
