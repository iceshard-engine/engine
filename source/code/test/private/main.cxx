#include <ice/allocator.hxx>
#include <ice/module_register.hxx>

#include <ice/resource.hxx>
#include <ice/resource_system.hxx>

#include <ice/log.hxx>
#include <ice/log_module.hxx>
#include <ice/assert.hxx>

//#include <core/memory.hxx>
//#include <core/allocators/proxy_allocator.hxx>
//#include <core/string.hxx>
//#include <core/stack_string.hxx>
//#include <core/string_view.hxx>
//#include <core/pod/array.hxx>
//#include <core/collections.hxx>
//#include <core/datetime/datetime.hxx>
//#include <core/platform/windows.hxx>
//
//#include <core/cexpr/stringid.hxx>
//#include <core/scope_exit.hxx>
//#include <core/debug/profiler.hxx>
//#include <core/pod/hash.hxx>
//#include <core/pod/algorithm.hxx>
//#include <core/clock.hxx>
//
//#include <resource/uri.hxx>
//#include <resource/resource_system.hxx>
//#include <resource/modules/dynlib_module.hxx>
//#include <resource/modules/filesystem_module.hxx>
//
//#include <core/message/buffer.hxx>
//#include <core/message/operations.hxx>
//
//#include <input_system/module.hxx>
//#include <input_system/message/app.hxx>
//#include <input_system/message/keyboard.hxx>
//#include <input_system/message/window.hxx>
//
//#include <asset_system/asset_system.hxx>
//#include <asset_system/assets/asset_config.hxx>
//#include <asset_system/assets/asset_shader.hxx>
//#include <asset_system/assets/asset_mesh.hxx>
//#include <asset_system/asset_module.hxx>
//
//#include <fmt/format.h>
//#include <application/application.hxx>
//
//#include <iceshard/math.hxx>
//#include <iceshard/module.hxx>
//#include <iceshard/engine.hxx>
//#include <iceshard/execution.hxx>
//#include <iceshard/frame.hxx>
//
//#include <iceshard/world/world.hxx>
//#include <iceshard/entity/entity_index.hxx>
//#include <iceshard/entity/entity_command_buffer.hxx>
//
//#include <iceshard/component/component_system.hxx>
//#include <iceshard/component/component_block.hxx>
//#include <iceshard/component/component_block_operation.hxx>
//#include <iceshard/component/component_block_allocator.hxx>
//#include <iceshard/component/component_archetype_index.hxx>
//#include <iceshard/component/component_archetype_iterator.hxx>
//
//#include <iceshard/action/action.hxx>
//#include <iceshard/action/action_trigger.hxx>
//#include <iceshard/action/action_trigger_data.hxx>
//#include <iceshard/action/action_system.hxx>
//
//#include <iceshard/renderer/render_pipeline.hxx>
//#include <iceshard/renderer/render_resources.hxx>
//#include <iceshard/renderer/render_pass.hxx>
//#include <iceshard/renderer/render_model.hxx>
//#include <iceshard/renderer/render_funcs.hxx>
//#include <iceshard/renderer/render_commands.hxx>
//#include <iceshard/renderer/render_buffers.hxx>
//
//#include <iceshard/render/render_stage.hxx>
//#include <iceshard/render/render_system.hxx>
//#include <iceshard/render/render_pass.hxx>
//
//#include <iceshard/material/material.hxx>
//#include <iceshard/material/material_system.hxx>
//
//#include <iceshard/input/input_mouse.hxx>
//#include <iceshard/input/input_keyboard.hxx>
//#include <iceshard/input/input_controller.hxx>
//
//#include <iceshard/ecs/model.hxx>
//#include <iceshard/ecs/transform.hxx>
//#include <iceshard/ecs/light.hxx>
//#include <iceshard/ecs/camera.hxx>
//#include <iceshard/math.hxx>
//
//#include <iceshard/debug/debug_module.hxx>
//#include <iceshard/debug/debug_system.hxx>
//
//class TileRenderer : public iceshard::ComponentSystem, public iceshard::RenderStageTaskFactory
//{
//public:
//    static constexpr core::math::u32 TileWidth = 64.f;
//    static constexpr core::math::u32 TileHeight = 64.f;
//
//    static constexpr core::math::f32 TilesetTileWidth = 16.f;
//    static constexpr core::math::f32 TilesetTileHeight = 16.f;
//
//    static constexpr core::math::vec2f TilesetTileUV{ 16.f / 368.f, 16.f / 224.f };
//
//    TileRenderer(iceshard::MaterialSystem& material_system, iceshard::renderer::RenderSystem& render_system) noexcept
//        : _material_system{ material_system }
//        , _render_system{ render_system }
//    {
//        _material_system.create_material("cotm.tileset"_sid,
//            iceshard::Material{
//                .texture_diffuse = "cotm/tileset_a"_sid,
//                .shader_vertex = "shaders/isometric/texture-vert"_sid,
//                .shader_fragment = "shaders/isometric/texture-frag"_sid,
//            },
//            iceshard::renderer::RenderPipelineLayout::Tiled
//        );
//
//        struct Vertice
//        {
//            ism::vec3f pos;
//            ism::vec3f norm;
//            ism::vec2f uv;
//        };
//
//        _tile_data = iceshard::renderer::create_buffer(
//            iceshard::renderer::api::BufferType::VertexBuffer,
//            sizeof(Vertice) * 4
//        );
//        _tile_indices = iceshard::renderer::create_buffer(
//            iceshard::renderer::api::BufferType::IndexBuffer,
//            sizeof(core::math::u16) * 6
//        );
//        _tile_instances = iceshard::renderer::create_buffer(
//            iceshard::renderer::api::BufferType::VertexBuffer,
//            sizeof(core::math::mat4x4) * 100
//        );
//
//        iceshard::renderer::Buffer buffers[] = {
//            _tile_data,
//            _tile_indices,
//            _tile_instances
//        };
//        iceshard::renderer::api::DataView views[core::size(buffers)];
//
//        auto buffers_arr = core::pod::array::create_view(buffers);
//        auto views_arr = core::pod::array::create_view(views);
//
//        iceshard::renderer::map_buffers(
//            buffers_arr,
//            views_arr
//        );
//
//        Vertice vertices[] = {
//            Vertice{
//                .pos = { 0.f, 0.f, 0.f },
//                .norm = { 0.f, 0.f, 0.f },
//                .uv = { 0.f, 1.f }
//            },
//            Vertice{
//                .pos = { 0.f, 1.f, 0.f },
//                .norm = { 0.f, 0.f, 0.f },
//                .uv = { 0.f, 0.f }
//            },
//            Vertice{
//                .pos = { 1.f, 1.f, 0.f },
//                .norm = { 0.f, 1.f, 0.f },
//                .uv = { 1.f, 0.f }
//            },
//            Vertice{
//                .pos = { 1.f, 0.f, 0.f },
//                .norm = { 0.f, 0.f, 0.f },
//                .uv = { 1.f, 1.f }
//            },
//        };
//
//        ism::u16 indices[] = {
//            0, 2, 1,
//            0, 3, 2
//        };
//
//        struct Instance
//        {
//            ism::vec2f pos;
//            ism::vec2u tile;
//        };
//
//        Instance instances[] = {
//            Instance{.pos = {0.f, 0.f}, .tile = {3, 1}},
//            Instance{.pos = {1.f, 0.f}, .tile = {3, 1}},
//            Instance{.pos = {0.f, 1.f}, .tile = {3, 1}},
//            Instance{.pos = {1.f, 1.f}, .tile = {3, 1}},
//            Instance{.pos = {1.f, 2.f}, .tile = {3, 1}},
//        };
//
//        memcpy(views[0].data, vertices, sizeof(vertices));
//        memcpy(views[1].data, indices, sizeof(indices));
//        memcpy(views[2].data, instances, sizeof(instances));
//
//        iceshard::renderer::unmap_buffers(
//            buffers_arr
//        );
//    }
//
//    auto render_task_factory() noexcept -> iceshard::RenderStageTaskFactory*
//    {
//        return this;
//    }
//
//    void create_render_tasks(
//        iceshard::Frame const& current,
//        iceshard::renderer::api::CommandBuffer cmds,
//        core::Vector<cppcoro::task<>>& task_list
//    ) noexcept override
//    {
//        task_list.push_back(draw_tiles(cmds));
//    }
//
//    void update(iceshard::Frame& frame) noexcept override
//    {
//        //core::pod::Array<int>& tiles = *frame.new_frame_object<core::pod::Array<int>>("demo.tiles"_sid);
//    }
//
//    auto draw_tiles(iceshard::renderer::CommandBuffer cmds) noexcept -> cppcoro::task<>
//    {
//        iceshard::MaterialResources mat;
//        _material_system.get_material("cotm.tileset"_sid, mat);
//
//        namespace cmd = iceshard::renderer::commands;
//        cmd::set_viewport(cmds, _viewport.x, _viewport.y);
//        cmd::set_scissor(cmds, _viewport.x, _viewport.y);
//
//        cmd::bind_pipeline(cmds, mat.pipeline);
//
//        iceshard::renderer::api::ResourceSet resources[] = {
//            mat.resource,
//            _render_system.get_resource_set("tiled.view-projection-clip"_sid)
//        };
//
//        cmd::bind_resource_sets(cmds, core::pod::array::create_view(resources));
//        cmd::bind_index_buffer(cmds, _tile_indices);
//
//        iceshard::renderer::Buffer buffers[] = {
//            _tile_data,
//            _tile_instances,
//        };
//        cmd::bind_vertex_buffers(cmds, core::pod::array::create_view(buffers));
//
//        cmd::draw_indexed(cmds, 6, 5, 0, 0, 0);
//        co_return;
//    }
//
//private:
//    iceshard::MaterialSystem& _material_system;
//    iceshard::renderer::RenderSystem& _render_system;
//
//    ism::vec2u _viewport{ 1280, 720 };
//
//    iceshard::renderer::Buffer _tile_data;
//    iceshard::renderer::Buffer _tile_indices;
//    iceshard::renderer::Buffer _tile_instances;
//};

#include <ice/memory/proxy_allocator.hxx>
#include <ice/resource_query.hxx>

#include <ice/asset.hxx>
#include <ice/asset_system.hxx>
#include <ice/asset_module.hxx>

#include <ice/render/render_model.hxx>
#include <ice/render/render_module.hxx>
#include <ice/render/render_queue.hxx>
#include <ice/render/render_pass.hxx>
#include <ice/render/render_swapchain.hxx>

#include <ice/engine.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_frame.hxx>
#include <ice/engine_module.hxx>
#include <ice/entity/entity_index.hxx>
#include <ice/entity/entity_tracker.hxx>
#include <ice/entity/entity_storage.hxx>
#include <ice/archetype/archetype_query.hxx>
#include <ice/archetype/archetype_index.hxx>
#include <ice/archetype/archetype_info.hxx>

#include <ice/gfx/gfx_device.hxx>

#include <ice/os/windows.hxx>
#include <ice/platform_app.hxx>
#include <ice/platform_window_surface.hxx>

#include <ice/input/input_tracker.hxx>
#include <ice/input/input_mouse.hxx>

struct TestComponent
{
    static constexpr ice::StringID Identifier = ice::stringid("ecs.test");

    float test;
};

struct BarComponent
{
    static constexpr ice::StringID Identifier = ice::stringid("ecs.bar");

    int bar;
};

struct FooComponent
{
    static constexpr ice::StringID Identifier = ice::stringid("ecs.foo");

    int xa[3];
};

class TestApp final : public ice::platform::App
{
public:
    TestApp(
        ice::Allocator& alloc,
        ice::UniquePtr<ice::EngineRunner> runner
    ) noexcept
        : _allocator{ alloc }
        , _runner{ ice::move(runner) }
        , _clock{ ice::clock::create_clock() }
        , _timeline{ ice::timeline::create_timeline(_clock) }
        , _input_tracker{ ice::input::create_default_input_tracker(_allocator, _clock) }
        , _input_events{ _allocator }
    {
        _input_tracker->register_device_type(
            ice::input::DeviceType::Mouse,
            ice::input::get_default_device_factory()
        );
    }

    ~TestApp() noexcept override
    {
        ICE_LOG(
            ice::LogSeverity::Debug, ice::LogTag::Game,
            "Elapsed time during testing {}s.",
            ice::timeline::elapsed(_timeline)
        );
    }

    void handle_inputs(
        ice::input::DeviceQueue const& device_events
    ) noexcept override
    {
        using namespace ice::input;

        ice::pod::array::clear(_input_events);
        _input_tracker->process_device_queue(device_events, _input_events);

        for (InputEvent const& ev : _input_events)
        {
            if (ev.identifier == input_identifier(DeviceType::Mouse, MouseInput::ButtonLeft) && (ev.value.button.state.clicked || ev.value.button.state.repeat))
            {
                ICE_LOG(
                    ice::LogSeverity::Debug, ice::LogTag::Game,
                    "Left Mouse button down for device {} with identifier {}",
                    static_cast<ice::u8>(ev.device),
                    static_cast<ice::u16>(ev.identifier)
                );
            }
        }
    }

    void update(
        ice::pod::Array<ice::platform::Event> const& events
    ) noexcept override
    {
        //ICE_LOG(
        //    ice::LogSeverity::Debug, ice::LogTag::Game,
        //    "Current frame memory consumption: {} bytes.",
        //    _runner->current_frame().memory_consumption()
        //);

        _runner->next_frame();

        ice::clock::update(_clock);
    }

private:
    ice::Allocator& _allocator;
    ice::UniquePtr<ice::EngineRunner> _runner;

    ice::SystemClock _clock = ice::clock::create_clock();
    ice::Timeline _timeline = ice::timeline::create_timeline(_clock);

    ice::UniquePtr<ice::input::InputTracker> _input_tracker;
    ice::pod::Array<ice::input::InputEvent> _input_events;
};

ice::i32 game_main(ice::Allocator& alloc, ice::ResourceSystem& resource_system)
{
    using ice::operator""_sid;
    using ice::operator""_uri;

    resource_system.mount("file://../source/data/config.json"_uri);

    ice::UniquePtr<ice::ModuleRegister> module_register = ice::create_default_module_register(alloc);
    module_register->load_module(
        alloc,
        ice::load_log_module,
        ice::unload_log_module
    );


    ice::Resource* const pipelines_module = resource_system.request("res://iceshard_pipelines.dll"_uri);
    ice::Resource* const engine_module = resource_system.request("res://iceshard.dll"_uri);
    ice::Resource* const vulkan_module = resource_system.request("res://vulkan_renderer.dll"_uri);

    ICE_ASSERT(pipelines_module != nullptr, "Missing `iceshard_pipelines.dll` module!");
    ICE_ASSERT(engine_module != nullptr, "Missing `iceshard.dll` module!");
    ICE_ASSERT(vulkan_module != nullptr, "Missing `vulkan_renderer.dll` module!");

    module_register->load_module(alloc, pipelines_module->location().path);
    module_register->load_module(alloc, engine_module->location().path);
    module_register->load_module(alloc, vulkan_module->location().path);


    ice::ResourceQuery resource_query;
    resource_system.query_changes(resource_query);
    resource_system.mount("file://mount.isr"_uri);

    ice::Resource* const mount_file = resource_system.request("file://mount.isr"_uri);
    if (mount_file != nullptr)
    {
        ICE_LOG(ice::LogSeverity::Info, ice::LogTag::Game, "Custom mount file found: {}\n", mount_file->location().path);
    }

    resource_system.query_changes(resource_query);
    resource_system.mount("dir://../source/data"_uri);
    resource_system.query_changes(resource_query);

    ice::UniquePtr<ice::AssetSystem> asset_system = ice::create_asset_system(alloc, resource_system);
    ice::load_asset_pipeline_modules(alloc, *module_register, *asset_system);

    asset_system->bind_resources(resource_query.objects);

    ice::Asset model_asset = asset_system->request(ice::AssetType::Mesh, "/mesh/box/box"_sid);

    ice::Data model_data;
    ice::asset_data(model_asset, model_data);

    ice::render::Model const* render_model = reinterpret_cast<ice::render::Model const*>(model_data.location);

    ICE_ASSERT(render_model != nullptr, "Unable to load test `render_model`");
    ICE_LOG(
        ice::LogSeverity::Debug, ice::LogTag::Game,
        "Test `model` has {} vertices and {} indices.",
        render_model->mesh_list[0].vertice_count,
        render_model->mesh_list[0].indice_count
    );

    //auto bar_task = []() -> ice::Task<int>
    //{
    //    co_return 42;
    //};

    //auto foo_task = [bar_task]() -> ice::Task<>
    //{
    //    ICE_LOG(
    //        ice::LogSeverity::Debug,
    //        ice::LogTag::Game,
    //        "foo task!"
    //    );

    //    int bar_result = co_await bar_task();

    //    ICE_LOG(
    //        ice::LogSeverity::Debug,
    //        ice::LogTag::Game,
    //        "bar task: {}",
    //        co_await bar_task()
    //    );

    //    co_return;
    //};

    //foo_task();

    asset_system->release(model_asset);

    ice::memory::ProxyAllocator entity_index_alloc{ alloc, "Entity-Index" };

    ice::EntityIndex entity_index{ entity_index_alloc, 100024 };
    ice::EntityTracker entity_tracker{ alloc, entity_index };

    //ice::ArchetypeBlockAllocator block_allocator{ alloc };
    //ice::UniquePtr<ice::ArchetypeIndex> archetype_index = ice::create_archetype_index(alloc);
    //ice::EntityStorage entity_storage{ alloc, *archetype_index, block_allocator };

    //ice::ArchetypeHandle base = archetype_index->register_archetype<>(&block_allocator);
    //ice::ArchetypeHandle test_bar = archetype_index->register_archetype<TestComponent, BarComponent>(&block_allocator);
    //ice::ArchetypeHandle foo = archetype_index->register_archetype<FooComponent>(&block_allocator);
    //ice::ArchetypeHandle foo_test_bar = archetype_index->register_archetype<FooComponent, TestComponent, BarComponent>(&block_allocator);

    ice::Entity player;
    if (entity_tracker.create_entity("ice.player"_sid, player))
    {
        entity_index.destroy(player);
    }

    if (entity_tracker.create_entity("ice.player"_sid, player))
    {
        ICE_LOG(
            ice::LogSeverity::Debug, ice::LogTag::Game,
            "Player entity created! ID: {}",
            ice::hash(player)
        );
    }

    entity_tracker.create_entity("ice.player"_sid, player);

    //entity_storage.set_archetype(player, foo);
    //entity_storage.change_archetype(player, test_bar);
    //entity_storage.change_archetype(player, foo_test_bar);
    //entity_storage.change_archetype(player, foo);
    //entity_storage.erase_data(player);

    ice::pod::Array<ice::Entity> entities{ alloc };
    ice::pod::array::resize(entities, 100);

    entity_index.create_many(entities);

    entity_index.destroy(entities[0]);
    entities[0] = entity_index.create(&entities);

    //entity_storage.set_archetype(
    //    entities[0], foo
    //);
    //entity_storage.set_archetype(
    //    entities[1], test_bar
    //);
    //entity_storage.set_archetype(
    //    entities[2], foo_test_bar
    //);
    //entity_storage.set_archetype(
    //    entities[3], foo
    //);

    static constexpr auto arch1 = ice::Archetype<TestComponent, FooComponent>{};
    //ice::ArchetypeComponent[] archetype_info = arch1.components;

    //std::is_same_v<ice::ComponentIdentifier<TestComponent>, ice::StringID const> ct = 333;
    //ice::ComponentQueryInfo<TestComponent*, FooComponent const&> query_info;
    //ice::ComponentQueryInfo<TestComponent*, BarComponent const&> query_info2;

    //ice::ComponentQuery query{ query_info, alloc, *archetype_index };
    //ice::ComponentQuery query2{ query_info2, alloc, *archetype_index };

    //auto entity_it = query.result_by_entity(alloc, entity_storage);
    //auto entity_it2 = query2.result_by_entity(alloc, entity_storage);

    //entity_it.for_each([](TestComponent* test, FooComponent const&)
    //{
    //    if (test)
    //    {
    //        test->test = 33.f;
    //    }
    //    else
    //    {
    //        ICE_LOG(
    //            ice::LogSeverity::Debug, ice::LogTag::Game,
    //            "Foo only entity"
    //        );
    //    }
    //});

    //entity_it2.for_each([](TestComponent* test, BarComponent const&)
    //{
    //    if (test)
    //    {
    //        if (test->test == 33.f)
    //        {
    //            ICE_LOG(
    //                ice::LogSeverity::Debug, ice::LogTag::Game,
    //                "Already set to 33.f"
    //            );
    //        }
    //        else
    //        {
    //            test->test = 33.f;
    //            ICE_LOG(
    //                ice::LogSeverity::Debug, ice::LogTag::Game,
    //                "Setting to 33.f"
    //            );
    //        }
    //    }
    //});
    //ice::ComponentQuery:: query.result_by_entity(alloc);

    //ice::pod::Array<ice::ArchetypeHandle> archetypes{ alloc };
    //archetype_index->find_archetypes(query.Constant_QueryInfo.index_query, archetypes);
    //archetypes._data;

    //ICE_LOG(
    //    ice::LogSeverity::Debug, ice::LogTag::Game,
    //    "Query component count: {}",
    //    query.Const_ComponentCount
    //);
    //ICE_LOG(
    //    ice::LogSeverity::Debug, ice::LogTag::Game,
    //    "Query component identifiers: {} ({:X}), {} ({:X})",
    //    ice::stringid_hint(query.Const_Identifiers[0]), ice::hash(query.Const_IdentifierHashes[0]),
    //    ice::stringid_hint(query.Const_Identifiers[1]), ice::hash(query.Const_IdentifierHashes[1])
    //);
    //ICE_LOG(
    //    ice::LogSeverity::Debug, ice::LogTag::Game,
    //    "Query component optionality: {}, {}",
    //    query.Const_ComponentOptional[0], query.Const_ComponentOptional[1]
    //);
    //ICE_LOG(
    //    ice::LogSeverity::Debug, ice::LogTag::Game,
    //    "Query component read-only: {}, {}",
    //    query.Const_ComponentReadOnly[0], query.Const_ComponentReadOnly[1]
    //);

    ICE_LOG(
        ice::LogSeverity::Debug, ice::LogTag::Game,
        "Array owns {} entities of {} total living!",
        entity_index.count_owned(&entities),
        entity_index.count()
    );
    ICE_LOG(
        ice::LogSeverity::Debug, ice::LogTag::Game,
        "Entity index is taking up {} bytes ~= {} kB!",
        entity_index_alloc.total_allocated(),
        entity_index_alloc.total_allocated() / 1024
    );


    ice::UniquePtr<ice::Engine> engine = ice::create_engine(alloc, *asset_system, *module_register);
    if (engine != nullptr)
    {
        ice::UniquePtr<ice::render::RenderDriver> render_driver = ice::render::create_render_driver(
            alloc, *module_register
        );

        ice::UniquePtr<ice::platform::WindowSurface> platform_surface = ice::platform::create_window_surface(
            alloc, ice::render::RenderDriverAPI::Vulkan
        );

        ice::render::SurfaceInfo surface_info;

        [[maybe_unused]]
        bool const query_success = platform_surface->query_details(surface_info);
        ICE_ASSERT(query_success, "Couldn't query surface details from platform surface!");

        ice::render::RenderSurface* render_surface = render_driver->create_surface(surface_info);

        ice::gfx::GfxPassCreateInfo gfx_pass_info[]{
            ice::gfx::GfxPassCreateInfo{
                .name = "default"_sid,
                .queue_flags = ice::render::QueueFlags::Graphics | ice::render::QueueFlags::Present
            }
        };

        ice::gfx::GfxDeviceCreateInfo gfx_device_info{
            .render_driver = render_driver.get(),
            .render_surface = render_surface,
            .pass_list = gfx_pass_info
        };

        ice::UniquePtr<ice::platform::Container> app = ice::platform::create_app_container(
            alloc,
            ice::make_unique<ice::platform::App, TestApp>(alloc, alloc,
                engine->create_runner(gfx_device_info)
            )
        );

        app->run();
        app = nullptr;

        render_driver->destroy_surface(render_surface);

        //ice::render::QueueCreateInfo queues{
        //    .graphics_queue_count = 1,
        //    .compute_queue_count = 1,
        //    .transfer_queue_count = 1,
        //    .presentation_queue = true
        //};

        //render_driver->create_queues(queues);


    }

    engine = nullptr;
    asset_system = nullptr;

    return 0;

//        auto& render_system = engine_instance->render_system();
//
//        using iceshard::renderer::RenderResource;
//        using iceshard::renderer::RenderResourceType;
//
//        // Debug UI module
//        core::memory::unique_pointer<iceshard::debug::DebugSystemModule> debugui_module{ nullptr, { alloc } };
//        core::memory::unique_pointer<iceshard::debug::DebugModule> engine_debugui{ nullptr, { alloc } };
//
//        if constexpr (core::build::is_release == false)
//        {
//            auto* debugui_module_location = resource_system.find(URN{ "debug_module.dll" });
//            if (debugui_module_location != nullptr)
//            {
//                debugui_module = iceshard::debug::load_debug_system_module(
//                    alloc,
//                    debugui_module_location->location().path,
//                    *engine_instance
//                );
//
//                engine_debugui = iceshard::debug::load_debug_module_from_handle(
//                    alloc, engine_module->native_handle(), debugui_module->debug_system()
//                );
//            }
//        }
//
//        fmt::print("IceShard engine revision: {}\n", engine_instance->revision());
//
//        [[maybe_unused]]
//        iceshard::World* world = engine_instance->world_manager().create_world("test-world"_sid);
//
//        auto arch_idx = world->service_provider()->archetype_index();
//
//        iceshard::Entity camera_entity = engine_instance->entity_manager().create();
//        arch_idx->add_component(camera_entity,
//            iceshard::component::Camera{
//                .position = { 0.0f, 0.0f, 1.0f },
//                .front = { 0.0f, 0.0f, 1.0f },
//                .yaw = -90.f,
//                .pitch = 0.0f,
//            }
//        );
//
//        arch_idx->add_component(camera_entity,
//            iceshard::component::ProjectionPerspective{
//                .fovy = 45.f
//            }
//        );
//
//        auto& material_system = engine_instance->material_system();
//
//        material_system.create_material("temp.backpack"_sid,
//            iceshard::Material{
//                .texture_diffuse = "temp/backpack_diffuse"_sid,
//                .shader_vertex = "shaders/debug/texture-vert"_sid,
//                .shader_fragment = "shaders/debug/texture-frag"_sid,
//            },
//            iceshard::renderer::RenderPipelineLayout::Textured
//        );
//
//        auto e = engine_instance->entity_manager().create();
//        arch_idx->add_component(
//            e, iceshard::component::Transform{ ism::identity<ism::mat4>() }
//        );
//        arch_idx->add_component(
//            e, iceshard::component::ModelName{ "temp/backpack"_sid }
//        );
//        arch_idx->add_component(
//            e, iceshard::component::Material{ "temp.backpack"_sid }
//        );
//
//        //arch_idx->add_component(camera_entity,
//        //    iceshard::component::ProjectionOrtographic{
//        //        .left_right = { 0.0f, 1280.f / TileRenderer::TileWidth },
//        //        .top_bottom = { 0.f, 720.f / TileRenderer::TileHeight },
//        //        .near_far = { 0.1f, 100.f },
//        //    }
//        //);
//
//        auto execution_instance = engine_instance->execution_instance();
//        iceshard::register_common_triggers(execution_instance->input_actions().trigger_database());
//
//        TileRenderer map_system{ material_system, engine_instance->render_system() };
//        engine_instance->services().add_system("demo.isometric"_sid, &map_system);
//
//        auto* const render_pass = execution_instance->render_system().render_pass("isc.render-pass.default"_sid);
//        if (render_pass != nullptr)
//        {
//            auto* const render_stage = render_pass->render_stage("isr.render-stage.opaque"_sid);
//
//            if (render_stage != nullptr)
//            {
//                render_stage->add_system_before("demo.isometric"_sid, "isc.system.static-mesh-renderer"_sid);
//            }
//        }
//
//        bool quit = false;
//        while (quit == false)
//        {
//            auto& frame = execution_instance->current_frame();
//
//            core::message::filter<input::message::AppExit>(frame.messages(), [&quit](auto const&) noexcept
//                {
//                    quit = true;
//                });
//
//            execution_instance->next_frame();
//        }
//
//        engine_instance->world_manager().destroy_world("test-world"_sid);
//
//        engine_debugui = nullptr;
//        debugui_module = nullptr;
//    }

}
