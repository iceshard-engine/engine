#include <core/memory.hxx>
#include <core/allocators/proxy_allocator.hxx>
#include <core/string.hxx>
#include <core/stack_string.hxx>
#include <core/string_view.hxx>
#include <core/pod/array.hxx>
#include <core/collections.hxx>
#include <core/datetime/datetime.hxx>
#include <core/platform/windows.hxx>

#include <core/cexpr/stringid.hxx>
#include <core/scope_exit.hxx>
#include <core/debug/profiler.hxx>
#include <core/pod/hash.hxx>
#include <core/pod/algorithm.hxx>
#include <core/clock.hxx>

#include <resource/uri.hxx>
#include <resource/resource_system.hxx>
#include <resource/modules/dynlib_module.hxx>
#include <resource/modules/filesystem_module.hxx>

#include <core/message/buffer.hxx>
#include <core/message/operations.hxx>

#include <input_system/module.hxx>
#include <input_system/message/app.hxx>
#include <input_system/message/keyboard.hxx>
#include <input_system/message/window.hxx>

#include <asset_system/asset_system.hxx>
#include <asset_system/assets/asset_config.hxx>
#include <asset_system/assets/asset_shader.hxx>
#include <asset_system/assets/asset_mesh.hxx>
#include <asset_system/asset_module.hxx>

#include <fmt/format.h>
#include <application/application.hxx>

#include <iceshard/math.hxx>
#include <iceshard/module.hxx>
#include <iceshard/engine.hxx>
#include <iceshard/execution.hxx>
#include <iceshard/frame.hxx>

#include <iceshard/world/world.hxx>
#include <iceshard/entity/entity_index.hxx>
#include <iceshard/entity/entity_command_buffer.hxx>

#include <iceshard/component/component_system.hxx>
#include <iceshard/component/component_block.hxx>
#include <iceshard/component/component_block_operation.hxx>
#include <iceshard/component/component_block_allocator.hxx>
#include <iceshard/component/component_archetype_index.hxx>
#include <iceshard/component/component_archetype_iterator.hxx>

#include <iceshard/action/action.hxx>
#include <iceshard/action/action_trigger.hxx>
#include <iceshard/action/action_trigger_data.hxx>
#include <iceshard/action/action_system.hxx>

#include <iceshard/renderer/render_pipeline.hxx>
#include <iceshard/renderer/render_resources.hxx>
#include <iceshard/renderer/render_pass.hxx>
#include <iceshard/renderer/render_model.hxx>
#include <iceshard/renderer/render_funcs.hxx>
#include <iceshard/renderer/render_commands.hxx>
#include <iceshard/renderer/render_buffers.hxx>

#include <iceshard/render/render_stage.hxx>
#include <iceshard/render/render_system.hxx>
#include <iceshard/render/render_pass.hxx>

#include <iceshard/material/material.hxx>
#include <iceshard/material/material_system.hxx>

#include <iceshard/input/input_mouse.hxx>
#include <iceshard/input/input_keyboard.hxx>
#include <iceshard/input/input_controller.hxx>

#include <iceshard/ecs/model.hxx>
#include <iceshard/ecs/transform.hxx>
#include <iceshard/ecs/light.hxx>
#include <iceshard/ecs/camera.hxx>
#include <iceshard/math.hxx>

#include <iceshard/debug/debug_module.hxx>
#include <iceshard/debug/debug_system.hxx>

class TileRenderer : public iceshard::ComponentSystem, public iceshard::RenderStageTaskFactory
{
public:
    static constexpr core::math::u32 TileWidth = 64.f;
    static constexpr core::math::u32 TileHeight = 64.f;

    static constexpr core::math::f32 TilesetTileWidth = 16.f;
    static constexpr core::math::f32 TilesetTileHeight = 16.f;

    static constexpr core::math::vec2f TilesetTileUV{ 16.f / 368.f, 16.f / 224.f };

    TileRenderer(iceshard::MaterialSystem& material_system, iceshard::renderer::RenderSystem& render_system) noexcept
        : _material_system{ material_system }
        , _render_system{ render_system }
    {
        _material_system.create_material("cotm.tileset"_sid,
            iceshard::Material{
                .texture_diffuse = "cotm/tileset_a"_sid,
                .shader_vertex = "shaders/isometric/texture-vert"_sid,
                .shader_fragment = "shaders/isometric/texture-frag"_sid,
            },
            iceshard::renderer::RenderPipelineLayout::Tiled
        );

        struct Vertice
        {
            ism::vec3f pos;
            ism::vec3f norm;
            ism::vec2f uv;
        };

        _tile_data = iceshard::renderer::create_buffer(
            iceshard::renderer::api::BufferType::VertexBuffer,
            sizeof(Vertice) * 4
        );
        _tile_indices = iceshard::renderer::create_buffer(
            iceshard::renderer::api::BufferType::IndexBuffer,
            sizeof(core::math::u16) * 6
        );
        _tile_instances = iceshard::renderer::create_buffer(
            iceshard::renderer::api::BufferType::VertexBuffer,
            sizeof(core::math::mat4x4) * 100
        );

        iceshard::renderer::Buffer buffers[] = {
            _tile_data,
            _tile_indices,
            _tile_instances
        };
        iceshard::renderer::api::DataView views[core::size(buffers)];

        auto buffers_arr = core::pod::array::create_view(buffers);
        auto views_arr = core::pod::array::create_view(views);

        iceshard::renderer::map_buffers(
            buffers_arr,
            views_arr
        );

        Vertice vertices[] = {
            Vertice{
                .pos = { 0.f, 0.f, 0.f },
                .norm = { 0.f, 0.f, 0.f },
                .uv = { 0.f, 1.f }
            },
            Vertice{
                .pos = { 0.f, 1.f, 0.f },
                .norm = { 0.f, 0.f, 0.f },
                .uv = { 0.f, 0.f }
            },
            Vertice{
                .pos = { 1.f, 1.f, 0.f },
                .norm = { 0.f, 1.f, 0.f },
                .uv = { 1.f, 0.f }
            },
            Vertice{
                .pos = { 1.f, 0.f, 0.f },
                .norm = { 0.f, 0.f, 0.f },
                .uv = { 1.f, 1.f }
            },
        };

        ism::u16 indices[] = {
            0, 2, 1,
            0, 3, 2
        };

        struct Instance
        {
            ism::vec2f pos;
            ism::vec2u tile;
        };

        Instance instances[] = {
            Instance{.pos = {0.f, 0.f}, .tile = {3, 1}},
            Instance{.pos = {1.f, 0.f}, .tile = {3, 1}},
            Instance{.pos = {0.f, 1.f}, .tile = {3, 1}},
            Instance{.pos = {1.f, 1.f}, .tile = {3, 1}},
            Instance{.pos = {1.f, 2.f}, .tile = {3, 1}},
        };

        memcpy(views[0].data, vertices, sizeof(vertices));
        memcpy(views[1].data, indices, sizeof(indices));
        memcpy(views[2].data, instances, sizeof(instances));

        iceshard::renderer::unmap_buffers(
            buffers_arr
        );
    }

    auto render_task_factory() noexcept -> iceshard::RenderStageTaskFactory*
    {
        return this;
    }

    void create_render_tasks(
        iceshard::Frame const& current,
        iceshard::renderer::api::CommandBuffer cmds,
        core::Vector<cppcoro::task<>>& task_list
    ) noexcept override
    {
        task_list.push_back(draw_tiles(cmds));
    }

    void update(iceshard::Frame& frame) noexcept override
    {
        //core::pod::Array<int>& tiles = *frame.new_frame_object<core::pod::Array<int>>("demo.tiles"_sid);
    }

    auto draw_tiles(iceshard::renderer::CommandBuffer cmds) noexcept -> cppcoro::task<>
    {
        iceshard::MaterialResources mat;
        _material_system.get_material("cotm.tileset"_sid, mat);

        namespace cmd = iceshard::renderer::commands;
        cmd::set_viewport(cmds, _viewport.x, _viewport.y);
        cmd::set_scissor(cmds, _viewport.x, _viewport.y);

        cmd::bind_pipeline(cmds, mat.pipeline);

        iceshard::renderer::api::ResourceSet resources[] = {
            mat.resource,
            _render_system.get_resource_set("tiled.view-projection-clip"_sid)
        };

        cmd::bind_resource_sets(cmds, core::pod::array::create_view(resources));
        cmd::bind_index_buffer(cmds, _tile_indices);

        iceshard::renderer::Buffer buffers[] = {
            _tile_data,
            _tile_instances,
        };
        cmd::bind_vertex_buffers(cmds, core::pod::array::create_view(buffers));

        cmd::draw_indexed(cmds, 6, 5, 0, 0, 0);
        co_return;
    }

private:
    iceshard::MaterialSystem& _material_system;
    iceshard::renderer::RenderSystem& _render_system;

    ism::vec2u _viewport{ 1280, 720 };

    iceshard::renderer::Buffer _tile_data;
    iceshard::renderer::Buffer _tile_indices;
    iceshard::renderer::Buffer _tile_instances;
};

int game_main(core::allocator& alloc, resource::ResourceSystem& resource_system)
{
    using resource::URN;
    using resource::URI;

    resource_system.mount(URI{ resource::scheme_file, "../source/data/config.json" });

    auto* engine_module_location = resource_system.find(URN{ "iceshard.dll" });
    IS_ASSERT(engine_module_location != nullptr, "Missing engine module!");

    if (auto engine_module = iceshard::load_engine_module(alloc, engine_module_location->location().path))
    {
        // Default file system mount points
        resource_system.flush_messages();
        resource_system.mount(URI{ resource::scheme_file, "mount.isr" });

        // Check for an user defined mounting file
        if (auto* mount_resource = resource_system.find(URI{ resource::scheme_file, "mount.isr" }))
        {
            fmt::print("Custom mount resource found: {}\n", mount_resource->location());
        }

        auto engine_instance = engine_module->create_instance(alloc, resource_system);

        resource_system.flush_messages();
        resource_system.mount(URI{ resource::scheme_directory, "../source/data" });

        // Prepare the asset system
        auto& asset_system = engine_instance->asset_system();

        auto* assimp_module_location = resource_system.find(URN{ "asset_module.dll" });
        auto assimp_module = iceshard::load_asset_module(alloc, assimp_module_location->location().path, asset_system);
        if (assimp_module == nullptr)
        {
            fmt::print("ERROR: Couldn't properly load `asset_module.dll`!\n");
        }

        asset_system.add_resolver(asset::default_resolver_mesh(alloc));
        asset_system.add_resolver(asset::default_resolver_shader(alloc));
        asset_system.add_loader(asset::default_loader_mesh(alloc));
        asset_system.add_loader(asset::default_loader_shader(alloc));
        asset_system.update();
        resource_system.flush_messages();

        // Prepare the render system
        auto& render_system = engine_instance->render_system();

        using iceshard::renderer::RenderResource;
        using iceshard::renderer::RenderResourceType;

        // Debug UI module
        core::memory::unique_pointer<iceshard::debug::DebugSystemModule> debugui_module{ nullptr, { alloc } };
        core::memory::unique_pointer<iceshard::debug::DebugModule> engine_debugui{ nullptr, { alloc } };

        if constexpr (core::build::is_release == false)
        {
            auto* debugui_module_location = resource_system.find(URN{ "debug_module.dll" });
            if (debugui_module_location != nullptr)
            {
                debugui_module = iceshard::debug::load_debug_system_module(
                    alloc,
                    debugui_module_location->location().path,
                    *engine_instance
                );

                engine_debugui = iceshard::debug::load_debug_module_from_handle(
                    alloc, engine_module->native_handle(), debugui_module->debug_system()
                );
            }
        }

        fmt::print("IceShard engine revision: {}\n", engine_instance->revision());

        [[maybe_unused]]
        iceshard::World* world = engine_instance->world_manager().create_world("test-world"_sid);

        auto arch_idx = world->service_provider()->archetype_index();

        iceshard::Entity camera_entity = engine_instance->entity_manager().create();
        arch_idx->add_component(camera_entity,
            iceshard::component::Camera{
                .position = { 0.0f, 0.0f, 1.0f },
                .front = { 0.0f, 0.0f, 1.0f },
                .yaw = -90.f,
                .pitch = 0.0f,
            }
        );

        arch_idx->add_component(camera_entity,
            iceshard::component::ProjectionPerspective{
                .fovy = 45.f
            }
        );

        auto& material_system = engine_instance->material_system();

        material_system.create_material("temp.backpack"_sid,
            iceshard::Material{
                .texture_diffuse = "temp/backpack_diffuse"_sid,
                .shader_vertex = "shaders/debug/texture-vert"_sid,
                .shader_fragment = "shaders/debug/texture-frag"_sid,
            },
            iceshard::renderer::RenderPipelineLayout::Textured
        );

        auto e = engine_instance->entity_manager().create();
        arch_idx->add_component(
            e, iceshard::component::Transform{ ism::identity<ism::mat4>() }
        );
        arch_idx->add_component(
            e, iceshard::component::ModelName{ "temp/backpack"_sid }
        );
        arch_idx->add_component(
            e, iceshard::component::Material{ "temp.backpack"_sid }
        );

        //arch_idx->add_component(camera_entity,
        //    iceshard::component::ProjectionOrtographic{
        //        .left_right = { 0.0f, 1280.f / TileRenderer::TileWidth },
        //        .top_bottom = { 0.f, 720.f / TileRenderer::TileHeight },
        //        .near_far = { 0.1f, 100.f },
        //    }
        //);

        auto execution_instance = engine_instance->execution_instance();
        iceshard::register_common_triggers(execution_instance->input_actions().trigger_database());

        TileRenderer map_system{ material_system, engine_instance->render_system() };
        engine_instance->services().add_system("demo.isometric"_sid, &map_system);

        auto* const render_pass = execution_instance->render_system().render_pass("isc.render-pass.default"_sid);
        if (render_pass != nullptr)
        {
            auto* const render_stage = render_pass->render_stage("isr.render-stage.opaque"_sid);

            if (render_stage != nullptr)
            {
                render_stage->add_system_before("demo.isometric"_sid, "isc.system.static-mesh-renderer"_sid);
            }
        }

        bool quit = false;
        while (quit == false)
        {
            auto& frame = execution_instance->current_frame();

            core::message::filter<input::message::AppExit>(frame.messages(), [&quit](auto const&) noexcept
                {
                    quit = true;
                });

            execution_instance->next_frame();
        }

        engine_instance->world_manager().destroy_world("test-world"_sid);

        engine_debugui = nullptr;
        debugui_module = nullptr;
    }

    return 0;
}
