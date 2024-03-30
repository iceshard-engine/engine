/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/framework_app.hxx>
#include <ice/framework_module.hxx>

#include <ice/module_register.hxx>
#include <ice/app.hxx>
#include <ice/app_info.hxx>

#include <ice/platform.hxx>
#include <ice/platform_core.hxx>
#include <ice/platform_threads.hxx>
#include <ice/platform_render_surface.hxx>
#include <ice/platform_window_surface.hxx>
#include <ice/platform_storage.hxx>

#include <ice/gfx/gfx_context.hxx>
#include <ice/gfx/gfx_runner.hxx>
#include <ice/gfx/gfx_queue.hxx>
#include <ice/gfx/gfx_stage.hxx>
#include <ice/gfx/gfx_graph.hxx>
#include <ice/gfx/gfx_graph_runtime.hxx>

#include <ice/render/render_module.hxx>
#include <ice/engine.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_module.hxx>
#include <ice/engine_shards.hxx>
#include <ice/engine_frame.hxx>
#include <ice/engine_state_tracker.hxx>
#include <ice/world/world_trait_archive.hxx>
#include <ice/world/world_trait_module.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/world/world_updater.hxx>

#include <ice/devui_module.hxx>
#include <ice/devui_context.hxx>

#include <ice/resource_tracker.hxx>
#include <ice/resource_provider.hxx>
#include <ice/asset_type_archive.hxx>
#include <ice/asset_storage.hxx>
#include <ice/asset_module.hxx>

#include <ice/input/device_event_queue.hxx>
#include <ice/input/input_tracker.hxx>

#include <ice/log_module.hxx>
#include <ice/log.hxx>
#include <ice/assert.hxx>

#include <ice/mem_allocator_proxy.hxx>
#include <ice/task_thread_pool.hxx>
#include <ice/task_utils.hxx>
#include <ice/task_scoped_container.hxx>
#include <ice/sync_manual_events.hxx>
#include <ice/path_utils.hxx>
#include <ice/string/heap_string.hxx>
#include <ice/profiler.hxx>
#include <ice/uri.hxx>

#include <thread>

#define USE_API_V1 0

struct FrameworkLog : ice::Module<ice::LogModule>
{
    IS_WORKAROUND_MODULE_INITIALIZATION(FrameworkLog);
};

template<typename T>
void destroy_object(T* obj) noexcept
{
    obj->alloc.destroy(obj);
}

struct ice::app::Config
{
    Config(ice::Allocator& alloc) noexcept
        : alloc{ alloc }
        , dev_dirs{ alloc }
    { }

    ice::Allocator& alloc;
    ice::String app_name = "Test App";

    struct DeveloperDirectories
    {
        DeveloperDirectories(ice::Allocator& alloc) noexcept
            : shaders{ alloc }
            , assets{ alloc }
        { }

        ice::HeapString<> shaders;
        ice::HeapString<> assets;
    } dev_dirs;
};

struct ice::app::State
{
    ice::Allocator& alloc;

    ice::ProxyAllocator resources_alloc;
    ice::ProxyAllocator modules_alloc;
    ice::ProxyAllocator gamework_alloc;
    ice::ProxyAllocator engine_alloc;

    ice::UniquePtr<ice::ResourceTracker> resources;
    ice::UniquePtr<ice::framework::Game> game;
    ice::UniquePtr<ice::ModuleRegister> modules;

    ice::UniquePtr<ice::AssetStorage> assets;
    ice::UniquePtr<ice::EngineStateTracker> states;
    ice::UniquePtr<ice::Engine> engine;
    ice::UniquePtr<ice::render::RenderDriver> renderer;
    ice::render::RenderSurface* render_surface; // TODO: Make into a UniquePtr

    struct ResourceProviders
    {
        ice::UniquePtr<ice::ResourceProvider> filesys;
        ice::UniquePtr<ice::ResourceProvider> modules;
    } providers;

    struct Debug
    {
        ice::UniquePtr<ice::DevUIContext> devui;
    } debug;

    struct Platform
    {
        ice::platform::Core* core;
        ice::platform::Threads* threads;
        ice::platform::RenderSurface* render_surface;
    } platform;

    State(ice::Allocator& alloc) noexcept
        : alloc{ alloc }
        , resources_alloc{ alloc, "Resources" }
        , modules_alloc{ alloc, "Modules" }
        , gamework_alloc{ alloc, "Gamework" }
        , engine_alloc{ alloc, "Engine" }
        , resources{ }
        , game{ ice::framework::create_game(gamework_alloc) }
        , modules{ ice::create_default_module_register(modules_alloc, true) }
        , render_surface{ }
        , providers{ }
        , platform{ .core = nullptr }
    {
        IPT_ZONE_SCOPED;

        using ice::platform::FeatureFlags;

        ice::platform::query_api(platform.core);
        ice::platform::query_api(platform.threads);
        ice::platform::query_api(platform.render_surface);

        ResourceTrackerCreateInfo const resource_info{
            .predicted_resource_count = 10000,
            .io_dedicated_threads = 1,
            .flags_io_complete = ice::TaskFlags{ },
            .flags_io_wait = ice::TaskFlags{ }
        };
        resources = ice::create_resource_tracker(resources_alloc, platform.threads->threadpool(), resource_info);
    }
};

struct ice::app::Runtime
{
    ice::Allocator& alloc;
    ice::ProxyAllocator render_alloc;
    ice::ProxyAllocator runtime_alloc;
    ice::ProxyAllocator app_alloc;

    ice::TaskQueue main_queue;
    ice::TaskScheduler main_scheduler;

    ice::UniquePtr<ice::EngineRunner> runner;
    ice::UniquePtr<ice::EngineFrame> frame;
    ice::UniquePtr<ice::gfx::GfxRunner> gfx_runner;
    ice::UniquePtr<ice::gfx::GfxGraph> gfx_rendergraph;
    ice::UniquePtr<ice::gfx::GfxGraphRuntime> gfx_rendergraph_runtime;

    ice::UniquePtr<ice::input::InputTracker> input_tracker;
    ice::Array<ice::input::InputEvent> input_events;

    ice::UniquePtr<ice::platform::WindowSurface> window_surface;
    ice::render::RenderDriver* render_driver;

    ice::SystemClock clock;
    ice::u32 close_attempts = 0;

    ice::ManualResetEvent logic_wait;
    ice::ManualResetEvent gfx_wait;

    bool is_starting = true;
    bool is_exiting = false;
    bool render_enabled = true;

    Runtime(ice::Allocator& alloc) noexcept
        : alloc{ alloc }
        , render_alloc{ alloc, "renderer" }
        , runtime_alloc{ alloc, "runtime" }
        , app_alloc{ alloc, "application" }
        , main_scheduler{ main_queue }
        , runner{ }
        , frame{ }
        , gfx_runner{ }
        , input_tracker{ }
        , input_events{ runtime_alloc }
        , window_surface{ }
    {
    }
};

void ice_init(
    ice::Allocator& alloc,
    ice::app::Factories& factories
) noexcept
{
    IPT_ZONE_SCOPED;
    using ice::operator|;
    using ice::app::Factories;
    using ice::platform::FeatureFlags;

    ice::Shard const platform_params[]{
        ice::platform::Shard_StorageAppName | "IceShard"
    };
    ice::Result const res = ice::platform::initialize_with_allocator(
        alloc,
        FeatureFlags::Core
        | FeatureFlags::StoragePaths
        | FeatureFlags::RenderSurface
        | FeatureFlags::Threads,
        platform_params
    );
    ICE_ASSERT(res == true, "Failed to initialize platform!");

    factories.factory_config = Factories::create_default<ice::app::Config>();
    factories.factory_state = Factories::create_default<ice::app::State>();
    factories.factory_runtime = Factories::create_default<ice::app::Runtime>();
}

void ice_args(
    ice::Allocator& alloc,
    ice::ParamList& params
) noexcept
{
    IPT_ZONE_SCOPED;
}

auto ice_setup(
    ice::Allocator& alloc,
    ice::ParamList const& params,
    ice::app::Config& config,
    ice::app::State& state
) noexcept -> ice::Result
{
    IPT_ZONE_SCOPED;
    using ice::operator""_uri;

    if (state.renderer != nullptr)
    {
        switch(state.renderer->state())
        {
            using ice::render::DriverState;
        case DriverState::Ready:
            ICE_LOG(ice::LogSeverity::Info, ice::LogTag::Engine, "GPU driver ready");
            return ice::app::S_ApplicationResume;
        case DriverState::Pending:
            ICE_LOG(ice::LogSeverity::Info, ice::LogTag::Engine, "GPU driver pending initialization");
            return ice::app::S_ApplicationSetupPending;
        case DriverState::Failed:
            ICE_LOG(ice::LogSeverity::Info, ice::LogTag::Engine, "GPU driver failed initialization");
        }
        return ice::app::E_FailedApplicationSetup;
    }

    ice::platform::StoragePaths* storage = nullptr;
    ice::platform::query_api(storage);
    ICE_ASSERT_CORE(storage != nullptr);

    ice::String dylib_path;
    ice::Array<ice::String> resource_paths{ alloc };
    if constexpr (ice::build::is_release == false && ice::build::is_windows)
    {
        dylib_path = ice::path::directory(ice::app::directory());
        config.dev_dirs.shaders = ice::app::workingdir();
        config.dev_dirs.assets = ice::app::workingdir();

        // Assumes the apps working-dir is in 'build' and no changes where done to shader compilation step
        ice::path::join(config.dev_dirs.shaders, "/obj/VkShaders/GFX-Vulkan-Unoptimized-vk-glslc-1-3/data");
        ice::path::join(config.dev_dirs.assets, "../source/data");
        ice::path::normalize(config.dev_dirs.shaders);
        ice::path::normalize(config.dev_dirs.assets);
        ice::string::push_back(config.dev_dirs.shaders, '/');
        ice::string::push_back(config.dev_dirs.assets, '/');
        ice::array::push_back(resource_paths, config.dev_dirs.assets);
        ice::array::push_back(resource_paths, config.dev_dirs.shaders);
    }
    else
    {
        dylib_path = storage->dylibs_location();
        ice::array::push_back(resource_paths, storage->data_locations());
    }

    ice::framework::Config game_config{
        .module_dir = dylib_path,
        .resource_dirs = resource_paths
    };

    state.game->on_config(game_config);

    // Load resources
    state.resources->attach_provider(
        ice::create_resource_provider(
            state.resources_alloc,
            game_config.resource_dirs,
            ice::build::current_platform == ice::build::System::WebApp
                ? nullptr
                : &state.platform.threads->threadpool()
        )
    );

    // There are no modules on the webapp platform
    if constexpr(ice::build::current_platform != ice::build::System::WebApp)
    {
        state.resources->attach_provider(
            ice::create_resource_provider_dlls(
                state.resources_alloc, game_config.module_dir
            )
        );
    }
    state.resources->sync_resources();

    if constexpr (ice::build::current_platform == ice::build::System::Android)
    {
        auto shaders_pak = state.resources->find_resource("urn://shaders.hsc"_uri);
        auto hailstorm = ice::create_resource_provider_hailstorm(
            state.resources_alloc, ice::resource_origin(shaders_pak)
        );

        state.resources->attach_provider(ice::move(hailstorm));
        state.resources->sync_resources();
    }

    ice::HeapString<> imgui_module = ice::resolve_dynlib_path(*state.resources, alloc, "imgui_module");
    state.modules->load_module(state.modules_alloc, imgui_module);

    if (ice::build::is_debug || ice::build::is_develop)
    {
        state.debug.devui = ice::create_devui_context(state.modules_alloc, *state.modules);
    }

    // Run game setup
    ice::framework::State const framework_state{
        .modules = *state.modules,
        .resources = *state.resources,
    };
    state.game->on_setup(framework_state);

    // Load everything to resume the game.
    ice::HeapString<> engine_module = ice::resolve_dynlib_path(*state.resources, state.alloc, "iceshard");
    state.modules->load_module(state.modules_alloc, engine_module);

    state.states = ice::create_state_tracker(state.alloc);
    ice::EngineCreateInfo engine_create_info{ .states = ice::move(state.states) };
    {
        ice::UniquePtr<ice::AssetTypeArchive> asset_types = ice::create_asset_type_archive(state.engine_alloc);
        ice::load_asset_type_definitions(state.engine_alloc, *state.modules, *asset_types);
        ice::AssetStorageCreateInfo const asset_storage_info{
            .resource_tracker = *state.resources,
            .task_scheduler = state.platform.threads->threadpool(),
            .task_flags = ice::TaskFlags{}
        };
        engine_create_info.assets = ice::create_asset_storage(state.resources_alloc, ice::move(asset_types), asset_storage_info);
    }

    {
        engine_create_info.traits = ice::create_default_trait_archive(state.engine_alloc, *engine_create_info.states);
        ice::load_trait_descriptions(state.engine_alloc, *state.modules, *engine_create_info.traits);
    }

    state.engine = ice::create_engine(state.engine_alloc, *state.modules, ice::move(engine_create_info));
    state.renderer = ice::render::create_render_driver(alloc, *state.modules);

    if (state.engine == nullptr || state.renderer == nullptr)
    {
        return ice::app::E_FailedApplicationSetup;
    }

    ice::render::SurfaceInfo surface_info{ };
    if (ice::platform::RenderSurface& surface = *state.platform.render_surface; surface.get_surface(surface_info) == false)
    {
        ice::platform::RenderSurfaceParams const surface_params{
            .driver = ice::render::RenderDriverAPI::Vulkan,
            .dimensions = { 1600, 1080 },
        };
        ice::Result result = surface.create(surface_params);
        ICE_ASSERT(
            result == ice::S_Success,
            "Failed to create render surface [ driver={}, dimensions={}x{} ]",
            ice::u32(surface_params.driver), surface_params.dimensions.x, surface_params.dimensions.y
        );

        bool const valid_surface_info = surface.get_surface(surface_info);
        ICE_ASSERT(valid_surface_info, "Failed to access surface info!");
    }

    state.render_surface = state.renderer->create_surface(surface_info);
    return ice::app::S_ApplicationSetupPending;
}

auto ice_resume(
    ice::app::Config const& config,
    ice::app::State& state,
    ice::app::Runtime& runtime
) noexcept -> ice::Result
{
    IPT_ZONE_SCOPED;
    if (runtime.is_starting)
    {
        ice::EngineRunnerCreateInfo const runner_create_info{
            .engine = *state.engine,
            .clock = runtime.clock,
            .schedulers = {
                .main = runtime.main_scheduler,
                .tasks = state.platform.threads->threadpool(),
            }
        };
        runtime.runner = ice::create_engine_runner(state.engine_alloc, *state.modules, runner_create_info);
        runtime.frame = ice::wait_for(runtime.runner->aquire_frame());
        runtime.clock = ice::clock::create_clock();


        using ice::operator|;
        using ice::operator""_sid;
        ice::gfx::GfxQueueDefinition queues[]{
            ice::gfx::GfxQueueDefinition {
                .name = "default"_sid,
                .flags = ice::render::QueueFlags::Graphics | ice::render::QueueFlags::Present
            },
            ice::gfx::GfxQueueDefinition {
                .name = "transfer"_sid,
                .flags = ice::render::QueueFlags::Transfer
            }
        };

        ice::platform::Threads* threads;
        bool const query_success = ice::platform::query_api(threads);
        ICE_ASSERT_CORE(query_success);

        ice::gfx::GfxRunnerCreateInfo const gfx_create_info{
            .engine = *state.engine,
            .driver = *state.renderer.get(),
            .surface = *state.render_surface,
            .render_queues = queues,
            .gfx_thread = threads->graphics()
        };

        runtime.gfx_runner = ice::create_gfx_runner(state.engine_alloc, *state.modules, gfx_create_info);
        runtime.input_tracker = ice::input::create_default_input_tracker(state.alloc, runtime.clock);
        runtime.input_tracker->register_device_type(ice::input::DeviceType::Mouse, ice::input::get_default_device_factory());
        runtime.input_tracker->register_device_type(ice::input::DeviceType::Keyboard, ice::input::get_default_device_factory());

        //runtime.gfx_rendergraph_runtime = state.game->rendergraph(runtime.gfx_runner->device());
        runtime.gfx_runner->update_rendergraph(state.game->rendergraph(runtime.gfx_runner->context()));
        runtime.gfx_wait.set();
    }

    state.game->on_resume(*state.engine);
    runtime.gfx_runner->on_resume();

    runtime.is_starting = false;
    return ice::app::S_ApplicationUpdate;
}

void ice_process_input_events(ice::Span<ice::input::InputEvent const> events, ice::ShardContainer& out_shards) noexcept
{
    for (ice::input::InputEvent const input_event : events)
    {
        ice::shards::push_back(out_shards, ice::ShardID_InputEvent | input_event);
    }
}

auto ice_update(
    ice::app::Config const& config,
    ice::app::State const& state,
    ice::app::Runtime& runtime
) noexcept -> ice::Result
{
    IPT_FRAME_MARK;
    IPT_ZONE_SCOPED;

    state.platform.core->refresh_events();
    ice::ShardContainer const& system_events = state.platform.core->system_events();

    bool const was_resized = ice::shards::contains(system_events, ice::platform::ShardID_WindowResized);
    bool const was_minimized = ice::shards::contains(system_events, ice::platform::ShardID_WindowMinimized);

    // Query platform events into the frame and input device handler.
    if (ice::shards::contains(system_events, ice::platform::Shard_AppQuit))
    {
        if (runtime.render_enabled)
        {
            runtime.gfx_wait.wait();
        }
        runtime.is_exiting = true;
        return ice::app::S_ApplicationExit;
    }

    ice::vec2i window_size;
    if (runtime.render_enabled)
    {
        if (ice::shards::inspect_last(system_events, ice::platform::ShardID_WindowResized, window_size))
        {
            runtime.gfx_wait.wait();
            runtime.gfx_runner->context().recreate_swapchain();
            runtime.gfx_runner->update_rendergraph(state.game->rendergraph(runtime.gfx_runner->context()));
        }
    }

    if (ice::shards::inspect_last(system_events, ice::platform::ShardID_WindowRestored, window_size))
    {
        runtime.render_enabled = true;
    }

    // Update the system clock
    ice::clock::update(runtime.clock);

    // Create Frame (blocks until previous frame is released?)
    ice::UniquePtr<ice::EngineFrame> new_frame = ice::wait_for(runtime.runner->aquire_frame());
    ICE_ASSERT(new_frame != nullptr, "Failed to aquire next frame!");

    ice::shards::push_back(new_frame->shards(), system_events._data);

    ice::array::clear(runtime.input_events);
    runtime.input_tracker->process_device_events(state.platform.core->input_events(), runtime.input_events);
    ice_process_input_events(runtime.input_events, new_frame->shards());

    // Send additional events
    state.game->on_update(*state.engine, *new_frame);
    state.engine->worlds().query_pending_events(new_frame->shards());

    state.engine->states().update_states(runtime.frame->shards(), new_frame->shards());

    // Run the frame update
    // TODO: Do it on a worker thread?
    // This will block (two frames alive)
    ice::manual_wait_for(
        runtime.runner->update_frame(*new_frame, *runtime.frame),
        runtime.logic_wait
    );

    if (runtime.render_enabled == false)
    {
        using namespace std;
        std::this_thread::sleep_for(1ms);
    }

    while(runtime.logic_wait.is_set() == false)
    {
        runtime.main_queue.process_all();
    }

    runtime.logic_wait.wait();
    runtime.logic_wait.reset();

    runtime.gfx_wait.wait();

    // Store this frame as the next frame for drawing
    runtime.runner->release_frame(
        ice::exchange(runtime.frame, ice::move(new_frame))
    );

    if (state.debug.devui != nullptr)
    {
        IPT_ZONE_SCOPED_NAMED("Runner Frame - Build Developer UI");
        state.debug.devui->update_widgets();
    }

    if (was_minimized)
    {
        runtime.render_enabled = false;
    }
    else if (runtime.render_enabled && !was_resized)
    {
        // Create GfxFrame (blocks until previous frame is released?)
        runtime.gfx_wait.reset();
        ice::manual_wait_for(
            runtime.gfx_runner->draw_frame(*runtime.frame, runtime.clock),
            runtime.gfx_wait
        );
    }

    return ice::app::S_ApplicationUpdate;
}

auto ice_suspend(
    ice::app::Config const& config,
    ice::app::State& state,
    ice::app::Runtime& runtime
) noexcept -> ice::Result
{
    IPT_ZONE_SCOPED;
    runtime.gfx_runner->on_suspend();
    state.game->on_suspend(*state.engine);

    if (runtime.is_exiting)
    {
        ice::shards::remove_all_of(
            runtime.frame->shards(),
            ice::ShardID_WorldActivate
        );

        ice::Array<ice::StringID> worlds{ state.alloc };
        state.engine->worlds().query_worlds(worlds);
        for (ice::StringID_Arg world : worlds)
        {
            ice::shards::push_back(runtime.frame->shards(), ice::ShardID_WorldDeactivate | ice::stringid_hash(world));
        }

        // Apply state events so we can already get the events for the next frame
        state.engine->states().update_states(runtime.frame->shards(), runtime.frame->shards());

        runtime.input_tracker.reset();
        runtime.runner->release_frame(ice::move(runtime.frame));

        runtime.gfx_rendergraph_runtime.reset();
        runtime.gfx_rendergraph.reset();
        runtime.gfx_runner.reset();
        state.renderer->destroy_surface(state.render_surface);

        runtime.runner.reset();
        return ice::app::S_ApplicationExit;
    }

    return ice::app::S_ApplicationResume;
}

auto ice_shutdown(
    ice::Allocator&,
    ice::ParamList const&,
    ice::app::Config const&,
    ice::app::State& state
) noexcept -> ice::Result
{
    ice::framework::State const framework_state{
        .modules = *state.modules,
        .resources = *state.resources,
    };
    state.game->on_shutdown(framework_state);

    state.renderer.reset();
    state.engine.reset();
    state.resources.reset();
    state.platform.render_surface->destroy();

    ice::platform::shutdown();
    return ice::S_Success;
}
