#include "terrain.hxx"
#include <ice/engine_runner.hxx>
#include <ice/engine_frame.hxx>
#include <ice/world/world.hxx>

#include <ice/input/input_event.hxx>
#include <ice/input/input_keyboard.hxx>
#include <ice/asset_system.hxx>

#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_resource_tracker.hxx>
#include <ice/gfx/gfx_model.hxx>
#include <ice/gfx/gfx_subpass.hxx>
#include <ice/gfx/gfx_camera.hxx>
#include <ice/gfx/gfx_task.hxx>
#include <ice/gfx/gfx_frame.hxx>

#include <ice/render/render_buffer.hxx>
#include <ice/render/render_pipeline.hxx>
#include <ice/render/render_pass.hxx>
#include <ice/render/render_resource.hxx>
#include <ice/render/render_command_buffer.hxx>
#include <ice/render/render_device.hxx>

#include <ice/log.hxx>

namespace ice::trait
{

    struct TerrainSettings
    {
        ice::f32 level_inner;
        ice::f32 level_outer;
    };

    struct Terrain::RenderCache
    {
        ice::u32 status;

        ice::render::Buffer _terrain_mesh_indices;
        ice::render::Buffer _terrain_mesh_vertices;
        ice::render::Buffer _terrain_instances;

        ice::render::ResourceSetLayout _terrain_resource_layout;
        ice::render::ResourceSet _terrain_resources;

        ice::render::PipelineLayout _terrain_pipeline_layout;
        ice::render::Pipeline _terrain_pipeline[2];

        ice::render::Shader _terrain_shaders[5];

        ice::render::ImageInfo _image_info;
        ice::render::Image _terrain_heightmap;
        ice::render::Sampler _terrain_sampler;

        ice::render::RenderDevice* _render_device;
        ice::render::Buffer _temp_transfer_buffer;
        ice::render::Buffer _camera_buffer;

        TerrainSettings _terrain_settings;
        ice::render::Buffer _terrain_settings_buffer;

        auto update_uniform_resources(
            ice::EngineRunner& runner
        ) noexcept -> ice::Task<>
        {
            using namespace render;

            //co_await runner.graphics_frame().frame_start();

            RenderDevice& device = runner.graphics_device().device();

            BufferUpdateInfo image_buffer_update[1]{
                BufferUpdateInfo{.buffer = _terrain_settings_buffer, .data = ice::data_view(_terrain_settings) },
            };
            _render_device->update_buffers(image_buffer_update);
            co_return;
        }

        auto update_all_resources(
            ice::EngineRunner& runner,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept -> ice::Task<>
        {
            using namespace render;

            //co_await runner.graphics_frame().frame_start();

            RenderDevice& device = gfx_device.device();

            BufferUpdateInfo image_buffer_update[1]{
                BufferUpdateInfo{.buffer = _temp_transfer_buffer, .data = { _image_info.data, _image_info.width * _image_info.height * 4 } },
            };
            _render_device->update_buffers(image_buffer_update);

            ResourceUpdateInfo update_resources[]{
                ResourceUpdateInfo{.image = _terrain_heightmap },
                ResourceUpdateInfo{.sampler = _terrain_sampler },
                ResourceUpdateInfo
                {
                    .uniform_buffer =
                    {
                        .buffer = _camera_buffer,
                        .offset = 0,
                        .size = sizeof(ice::gfx::GfxCameraUniform),
                    }
                },
                ResourceUpdateInfo
                {
                    .uniform_buffer =
                    {
                        .buffer = _terrain_settings_buffer,
                        .offset = 0,
                        .size = sizeof(TerrainSettings),
                    }
                },
            };

            ResourceSetUpdateInfo update_terrain_set[]{
                ResourceSetUpdateInfo{
                    .resource_set = _terrain_resources,
                    .resource_type = ResourceType::SampledImage,
                    .binding_index = ice::gfx::GfxSubpass_Terrain::ResConst_TerrainHeightmapBinding,
                    .array_element = 0,
                    .resources = { update_resources + 0, 1 }
                },
                ResourceSetUpdateInfo{
                    .resource_set = _terrain_resources,
                    .resource_type = ResourceType::Sampler,
                    .binding_index = ice::gfx::GfxSubpass_Terrain::ResConst_TerrainSamplerBinding,
                    .array_element = 0,
                    .resources = { update_resources + 1, 1 }
                },
                ResourceSetUpdateInfo{
                    .resource_set = _terrain_resources,
                    .resource_type = ResourceType::UniformBuffer,
                    .binding_index = ice::gfx::GfxSubpass_Terrain::ResConst_TerrainUniformBinding,
                    .array_element = 0,
                    .resources = { update_resources + 2, 1 }
                },
                ResourceSetUpdateInfo{
                    .resource_set = _terrain_resources,
                    .resource_type = ResourceType::UniformBuffer,
                    .binding_index = ice::gfx::GfxSubpass_Terrain::ResConst_TerrainUniformBinding + 1,
                    .array_element = 0,
                    .resources = { update_resources + 3, 1 }
                },
            };

            _render_device->update_resourceset(update_terrain_set);

            ice::gfx::GfxTaskCommands& cmds = co_await runner.graphics_frame().frame_commands("default"_sid);

            cmds.update_texture(_terrain_heightmap, _temp_transfer_buffer, { _image_info.width, _image_info.height });
        }
    };

    void terrain_update_render_cache(
        Terrain::RenderCache& cache,
        ice::AssetSystem& assets,
        ice::render::RenderDevice& device
    )
    {

        using namespace ice::render;

        //ice::Asset terrain_vert = assets.request(AssetType::Shader, "/shaders/terrain/terrain-vert"_sid);
        //ice::Asset terrain_frag = assets.request(AssetType::Shader, "/shaders/terrain/terrain-frag"_sid);

        //cache._terrain_mesh_vertices = device.create_buffer(BufferType::Vertex, 256);
        //cache._terrain_mesh_indices = device.create_buffer(BufferType::Index, 256);
        //cache._terrain_instances = device.create_buffer(BufferType::Vertex, sizeof(ice::mat4) * 256);
    }

    Terrain::Terrain(
        ice::Allocator& alloc,
        ice::Engine& engine
    ) noexcept
        : _engine{ engine }
        , _asset_system{ _engine.asset_system() }
        , _render_cache{ ice::make_unique<RenderCache>(alloc) }
        //, _update_stages{ alloc }
    {
        _render_cache->status = 1;
    }

    void Terrain::on_activate(
        ice::Engine&,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        using namespace ice::gfx;
        using namespace ice::render;

        AssetSystem& asset_system = _engine.asset_system();
        Asset vert_shader = asset_system.request(AssetType::Shader, "/shaders/terrain/terrain-vert"_sid);
        Asset tes_ctrl_shader = asset_system.request(AssetType::Shader, "/shaders/terrain/terrain-tes-ctrl"_sid);
        Asset tes_eval_shader = asset_system.request(AssetType::Shader, "/shaders/terrain/terrain-tes-eval"_sid);
        Asset geom_shader = asset_system.request(AssetType::Shader, "/shaders/terrain/terrain-geom"_sid);
        Asset frag_shader = asset_system.request(AssetType::Shader, "/shaders/terrain/terrain-frag"_sid);

        Asset heightmap_image = asset_system.request(AssetType::Texture, "/terrain/map"_sid);

        Data vert_shader_data;
        Data tesc_shader_data;
        Data tese_shader_data;
        Data geom_shader_data;
        Data frag_shader_data;

        {
            Data temp_data;
            ice::asset_data(vert_shader, temp_data);
            vert_shader_data = *reinterpret_cast<Data const*>(temp_data.location);

            ice::asset_data(tes_ctrl_shader, temp_data);
            tesc_shader_data = *reinterpret_cast<Data const*>(temp_data.location);

            ice::asset_data(tes_eval_shader, temp_data);
            tese_shader_data = *reinterpret_cast<Data const*>(temp_data.location);

            ice::asset_data(geom_shader, temp_data);
            geom_shader_data = *reinterpret_cast<Data const*>(temp_data.location);

            ice::asset_data(frag_shader, temp_data);
            frag_shader_data = *reinterpret_cast<Data const*>(temp_data.location);

            ice::asset_data(heightmap_image, temp_data);
            _render_cache->_image_info = *reinterpret_cast<ImageInfo const*>(temp_data.location);
        }

        GfxDevice& gfx_device = runner.graphics_device();
        GfxResourceTracker& gfx_resources = gfx_device.resource_tracker();

        Renderpass renderpass = find_resource<Renderpass>(gfx_resources, "renderpass.default"_sid);
        ice::render::Buffer camera_buffer = find_resource<ice::render::Buffer>(gfx_resources, "uniform_buffer.camera"_sid);

        RenderDevice& render_device = gfx_device.device();

        _render_cache->_render_device = &render_device;
        _render_cache->_terrain_shaders[0] = render_device.create_shader(ShaderInfo{ .shader_data = vert_shader_data });
        _render_cache->_terrain_shaders[1] = render_device.create_shader(ShaderInfo{ .shader_data = tesc_shader_data });
        _render_cache->_terrain_shaders[2] = render_device.create_shader(ShaderInfo{ .shader_data = tese_shader_data });
        _render_cache->_terrain_shaders[3] = render_device.create_shader(ShaderInfo{ .shader_data = geom_shader_data });
        _render_cache->_terrain_shaders[4] = render_device.create_shader(ShaderInfo{ .shader_data = frag_shader_data });

        _render_cache->_terrain_pipeline_layout = find_resource<PipelineLayout>(gfx_resources, GfxSubpass_Terrain::ResName_PipelineLayout);
        _render_cache->_terrain_resource_layout = find_resource<ResourceSetLayout>(gfx_resources, GfxSubpass_Terrain::ResName_ResourceLayout);

        _render_cache->_temp_transfer_buffer = find_resource<ice::render::Buffer>(gfx_resources, "temp.buffer.image_transfer"_sid);
        _render_cache->_terrain_heightmap = render_device.create_image(
            _render_cache->_image_info, { }
        );
        _render_cache->_camera_buffer = camera_buffer;


        //ResourceSetLayout rs_layout = find_resource<ResourceSetLayout>(gfx_resources, GfxSubpass_Primitives::ResName_ResourceLayout);

        //ResourceSetLayoutBinding terrain_resource_bindings[2]{
        //    ResourceSetLayoutBinding {
        //        .binding_index = 0,
        //        .resource_count = 1,
        //        .resource_type = ResourceType::UniformBuffer,
        //        .shader_stage_flags = ShaderStageFlags::VertexStage | ShaderStageFlags::FragmentStage
        //    },
        //    ResourceSetLayoutBinding {
        //        .binding_index = 1,
        //        .resource_count = 1,
        //        .resource_type = ResourceType::UniformBuffer,
        //        .shader_stage_flags = ShaderStageFlags::VertexStage | ShaderStageFlags::FragmentStage
        //    },
        //};

        //_render_cache->_terrain_resource_layout = render_device.create_resourceset_layout(
        //    terrain_resource_bindings
        //);

        //PipelineLayoutInfo terrain_pipeline_layout{
        //    .push_constants = { },
        //    .resource_layouts = ice::Span<ResourceSetLayout>{ &_render_cache->_terrain_resource_layout, 1 }
        //};

        //_render_cache->_terrain_pipeline_layout = render_device.create_pipeline_layout(
        //    terrain_pipeline_layout
        //);

        //render_device.create_resourcesets(
        //    { &_render_cache->_terrain_resource_layout, 1 },
        //    { &_render_cache->_terrain_resources, 1 }
        //);

        render_device.create_resourcesets(
            { &_render_cache->_terrain_resource_layout, 1 },
            { &_render_cache->_terrain_resources , 1 }
        );

        SamplerInfo sampler_info{
            .min_filter = SamplerFilter::Nearest,
            .mag_filter = SamplerFilter::Nearest,
            .address_mode = {
                .u = SamplerAddressMode::Repeat,
                .v = SamplerAddressMode::Repeat,
                .w = SamplerAddressMode::Repeat,
            },
            .mip_map_mode = SamplerMipMapMode::Nearest,
        };

        _render_cache->_terrain_sampler = render_device.create_sampler(sampler_info);

        ShaderStageFlags const shader_stages[]{
            ShaderStageFlags::VertexStage,
            ShaderStageFlags::TesselationControlStage,
            ShaderStageFlags::TesselationEvaluationStage,
            ShaderStageFlags::GeometryStage,
            ShaderStageFlags::FragmentStage,
        };

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
            //ShaderInputBinding{
            //    .binding = 1,
            //    .stride = sizeof(ice::mat4),
            //    .instanced = 1,
            //    .attributes = { attribs + 3, 4 }
            //}
        };

        PipelineInfo terrain_pipeline{
            .layout = _render_cache->_terrain_pipeline_layout,
            .renderpass = renderpass,
            .shaders = _render_cache->_terrain_shaders,
            .shaders_stages = { shader_stages },
            .shader_bindings = shader_bindings,
            .primitive_topology = PrimitiveTopology::PatchList,
            .cull_mode = CullMode::BackFace,
            .front_face = FrontFace::CounterClockWise,
            .subpass_index = 1,
            .depth_test = true,
        };

        _render_cache->_terrain_pipeline[0] = render_device.create_pipeline(
            terrain_pipeline
        );

        terrain_pipeline = PipelineInfo{
            .layout = _render_cache->_terrain_pipeline_layout,
            .renderpass = renderpass,
            .shaders = _render_cache->_terrain_shaders,
            .shaders_stages = { shader_stages },
            .shader_bindings = shader_bindings,
            .primitive_topology = PrimitiveTopology::PatchList,
            .cull_mode = CullMode::Disabled,
            .front_face = FrontFace::CounterClockWise,
            .subpass_index = 1,
            .depth_test = false,
        };

        _render_cache->_terrain_pipeline[1] = render_device.create_pipeline(
            terrain_pipeline
        );

        ice::gfx::Vertice vertices[]{
            Vertice{ .pos = { -50.f, 0.f, -50.f }, .norm = { 0.f, 1.f, 0.f }, .uv = { 0.f, 0.f } },
            Vertice{ .pos = { -50.f, 0.f, 50.f }, .norm = { 0.f, 1.f, 0.f }, .uv = { 0.f, 1.f } },
            Vertice{ .pos = { 50.f, 0.f, 50.f }, .norm = { 0.f, 1.f, 0.f }, .uv = { 1.f, 1.f } },
            Vertice{ .pos = { 50.f, 0.f, -50.f }, .norm = { 0.f, 1.f, 0.f }, .uv = { 1.f, 0.f } },
        };

        ice::u16 indices[]{
            0, 1, 2, 3
            //0, 2, 3,
        };

        _render_cache->_terrain_mesh_indices = render_device.create_buffer(BufferType::Index, sizeof(indices));
        _render_cache->_terrain_mesh_vertices = render_device.create_buffer(BufferType::Vertex, sizeof(vertices));
        _render_cache->_terrain_settings_buffer = render_device.create_buffer(BufferType::Uniform, sizeof(TerrainSettings));

        _render_cache->_terrain_settings.level_inner = 2;
        _render_cache->_terrain_settings.level_outer = 2;

        BufferUpdateInfo buffer_updates[]{
            BufferUpdateInfo{ .buffer = _render_cache->_terrain_mesh_indices, .data = ice::data_view(indices, sizeof(indices)) },
            BufferUpdateInfo{ .buffer = _render_cache->_terrain_mesh_vertices, .data = ice::data_view(vertices, sizeof(vertices)) },
            BufferUpdateInfo{ .buffer = _render_cache->_terrain_settings_buffer, .data = ice::data_view(_render_cache->_terrain_settings) },
        };

        render_device.update_buffers(buffer_updates);

        runner.execute_task(
            _render_cache->update_all_resources(
                runner,
                runner.graphics_device()
            ), EngineContext::GraphicsFrame
        );
    }

    void Terrain::on_deactivate(
        ice::Engine&,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        using namespace ice::gfx;
        using namespace ice::render;

        GfxDevice& gfx_device = runner.graphics_device();
        GfxResourceTracker& gfx_resources = gfx_device.resource_tracker();

        RenderDevice& render_device = gfx_device.device();

        render_device.destroy_resourcesets({ &_render_cache->_terrain_resources, 1 });

        render_device.destroy_image(_render_cache->_terrain_heightmap);
        render_device.destroy_sampler(_render_cache->_terrain_sampler);

        render_device.destroy_buffer(_render_cache->_terrain_mesh_vertices);
        render_device.destroy_buffer(_render_cache->_terrain_mesh_indices);
        render_device.destroy_buffer(_render_cache->_terrain_settings_buffer);

        render_device.destroy_pipeline(_render_cache->_terrain_pipeline[0]);
        render_device.destroy_pipeline(_render_cache->_terrain_pipeline[1]);

        render_device.destroy_shader(_render_cache->_terrain_shaders[0]);
        render_device.destroy_shader(_render_cache->_terrain_shaders[1]);
        render_device.destroy_shader(_render_cache->_terrain_shaders[2]);
        render_device.destroy_shader(_render_cache->_terrain_shaders[3]);
        render_device.destroy_shader(_render_cache->_terrain_shaders[4]);

        //render_device.destroy_pipeline_layout(_render_cache->_terrain_pipeline_layout);
        //render_device.destroy_resourceset_layout(_render_cache->_terrain_resource_layout);
    }

    void Terrain::on_update (
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        using namespace ice::input;

        static bool inner_values = true;
        ice::f32* value_ptr[2]{
            &_render_cache->_terrain_settings.level_outer,
            &_render_cache->_terrain_settings.level_inner,
        };

        for (InputEvent const& ev : frame.input_events())
        {
            switch (ev.identifier)
            {
            case input_identifier(DeviceType::Keyboard, KeyboardKey::KeyG):
                if (ev.value.button.state.clicked)
                {
                    inner_values = !inner_values;
                }
                break;
            case input_identifier(DeviceType::Keyboard, KeyboardKey::KeyP):
                if (ev.value.button.state.clicked || ev.value.button.state.repeat)
                {
                    _debug_pl = !_debug_pl;
                }
                break;
            case input_identifier(DeviceType::Keyboard, KeyboardKey::KeyI):
                if (ev.value.button.state.clicked || ev.value.button.state.repeat)
                {
                    *value_ptr[0] *= 2.f;
                    if (*value_ptr[0] > 64.f)
                    {
                        *value_ptr[0] = 64.f;
                    }

                    *value_ptr[1] *= 2.f;
                    if (*value_ptr[1] > 64.f)
                    {
                        *value_ptr[1] = 64.f;
                    }
                }
                break;
            case input_identifier(DeviceType::Keyboard, KeyboardKey::KeyU):
                if (ev.value.button.state.clicked || ev.value.button.state.repeat)
                {
                    *value_ptr[0] /= 2.f;
                    if (*value_ptr[0] < 1.f)
                    {
                        *value_ptr[0] = 1.f;
                    }

                    *value_ptr[1] /= 2.f;
                    if (*value_ptr[1] < 1.f)
                    {
                        *value_ptr[1] = 1.f;
                    }
                }
                break;
            }
        }

        runner.execute_task(
            _render_cache->update_uniform_resources(runner),
            EngineContext::GraphicsFrame
        );
    }

    void Terrain::record_commands(
        ice::EngineFrame const& frame,
        ice::render::CommandBuffer cmds,
        ice::render::RenderCommands& render_commands
    ) noexcept
    {
        render_commands.bind_pipeline(cmds, _render_cache->_terrain_pipeline[_debug_pl]);
        render_commands.bind_resource_set(cmds, _render_cache->_terrain_pipeline_layout, _render_cache->_terrain_resources, 1);
        render_commands.bind_vertex_buffer(cmds, _render_cache->_terrain_mesh_vertices, 0);
        render_commands.bind_index_buffer(cmds, _render_cache->_terrain_mesh_indices);
        render_commands.draw_indexed(cmds, 4, 1);
    }

} // namespace ice::trait
