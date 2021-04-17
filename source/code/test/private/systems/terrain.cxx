#include "terrain.hxx"
#include <ice/engine_runner.hxx>
#include <ice/engine_frame.hxx>
#include <ice/world/world.hxx>

#include <ice/asset_system.hxx>

#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_resource_tracker.hxx>

#include <ice/render/render_buffer.hxx>
#include <ice/render/render_pipeline.hxx>
#include <ice/render/render_pass.hxx>
#include <ice/render/render_resource.hxx>
#include <ice/render/render_command_buffer.hxx>
#include <ice/render/render_device.hxx>

#include <ice/log.hxx>

namespace ice::trait
{

    struct Terrain::RenderCache
    {
        ice::u32 status;

        ice::render::Buffer _terrain_mesh_indices;
        ice::render::Buffer _terrain_mesh_vertices;
        ice::render::Buffer _terrain_instances;

        ice::render::ResourceSetLayout _terrain_resource_layout;
        ice::render::ResourceSet _terrain_resources;

        ice::render::PipelineLayout _terrain_pipeline_layout;
        ice::render::Pipeline _terrain_pipeline;
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
    {
        _render_cache->status = 1;
    }

    void Terrain::on_activate(
        ice::EngineRunner& runner,
        ice::World& world
    ) noexcept
    {
        using namespace ice::gfx;
        using namespace ice::render;

        AssetSystem& asset_system = _engine.asset_system();
        Asset vert_shader = asset_system.request(AssetType::Shader, "/shaders/terrain/terrain-vert"_sid);
        Asset frag_shader = asset_system.request(AssetType::Shader, "/shaders/terrain/terrain-frag"_sid);

        Data vert_shader_data;
        Data frag_shader_data;

        {
            Data temp_data;
            ice::asset_data(vert_shader, temp_data);
            vert_shader_data = *reinterpret_cast<Data const*>(temp_data.location);

            ice::asset_data(frag_shader, temp_data);
            frag_shader_data = *reinterpret_cast<Data const*>(temp_data.location);
        }

        GfxDevice& gfx_device = runner.graphics_device();
        GfxResourceTracker& gfx_resources = gfx_device.resource_tracker();

        GfxResource gfx_renderpass = gfx_resources.find_resource("renderpass.default"_sid, GfxResource::Type::Renderpass);


        RenderDevice& render_device = gfx_device.device();

        ResourceSetLayoutBinding terrain_resource_bindings[1]{
            ResourceSetLayoutBinding {
                .binding_index = 0,
                .resource_count = 1,
                .resource_type = ResourceType::UniformBuffer,
                .shader_stage_flags = ShaderStageFlags::VertexStage | ShaderStageFlags::FragmentStage
            },
        };

        _render_cache->_terrain_resource_layout = render_device.create_resourceset_layout(
            terrain_resource_bindings
        );

        PipelineLayoutInfo terrain_pipeline_layout{
            .push_constants = { },
            .resource_layouts = ice::Span<ResourceSetLayout>{ &_render_cache->_terrain_resource_layout, 1 }
        };

        _render_cache->_terrain_pipeline_layout = render_device.create_pipeline_layout(
            terrain_pipeline_layout
        );

        PipelineInfo terrain_pipeline{
            .layout = _render_cache->_terrain_pipeline_layout,
            .renderpass = gfx_renderpass.value.renderpass,
            .shaders = { },
            .shaders_stages = { },
            .shader_bindings = { },
            .subpass_index = 1,
            .depth_test = false,
        };

        //_render_cache->_terrain_pipeline = render_device.create_pipeline(
        //    terrain_pipeline
        //);
    }

    void Terrain::on_deactivate(
        ice::EngineRunner& runner,
        ice::World& world
    ) noexcept
    {
        using namespace ice::gfx;
        using namespace ice::render;

        GfxDevice& gfx_device = runner.graphics_device();
        GfxResourceTracker& gfx_resources = gfx_device.resource_tracker();

        RenderDevice& render_device = gfx_device.device();

        render_device.destroy_pipeline_layout(_render_cache->_terrain_pipeline_layout);
        render_device.destroy_resourceset_layout(_render_cache->_terrain_resource_layout);
    }

    void Terrain::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::World& world
    ) noexcept
    {
        ice::render::RenderDevice& device = runner.graphics_device().device();

        ICE_LOG(
            ice::LogSeverity::Debug, ice::LogTag::Module,
            "input events: {}", ice::size(runner.previous_frame().input_events())
        );

        terrain_update_render_cache(*_render_cache, _asset_system, device);
    }

} // namespace ice::trait
