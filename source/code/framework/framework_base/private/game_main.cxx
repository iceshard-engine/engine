/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/game_framework.hxx>
#include <ice/game_module.hxx>

#include <ice/module_register.hxx>
#include <ice/app.hxx>
#include <ice/app_info.hxx>

#include <ice/platform_app.hxx>
#include <ice/platform_window_surface.hxx>

#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_runner.hxx>

#include <ice/render/render_module.hxx>
#include <ice/engine.hxx>
#include <ice/engine_module.hxx>
#include <ice/world/world_trait_archive.hxx>
#include <ice/world/world_trait_module.hxx>

#include <ice/devui/devui_module.hxx>
#include <ice/devui/devui_system.hxx>

#include <ice/resource_tracker.hxx>
#include <ice/resource_provider.hxx>
#include <ice/asset_type_archive.hxx>
#include <ice/asset_storage.hxx>
#include <ice/asset_module.hxx>

#include <ice/log_module.hxx>
#include <ice/log.hxx>
#include <ice/assert.hxx>

#include <ice/mem_allocator_proxy.hxx>
#include <ice/task_thread_pool.hxx>
#include <ice/path_utils.hxx>
#include <ice/string/heap_string.hxx>

template<typename T>
void destroy_object(T* obj) noexcept
{
    obj->alloc.destroy(obj);
}

struct ice::app::Config
{
    Config(ice::Allocator& alloc) noexcept
        : alloc{ alloc }
        , dirs{ alloc }
    {
    }

    ice::Allocator& alloc;
    ice::String app_name = "My App";

    struct Directories
    {
        Directories(ice::Allocator& alloc) noexcept
            : modules{ }
            , shaders{ alloc }
            , assets{ alloc }
        { }

        ice::String modules;
        ice::HeapString<> shaders;
        ice::HeapString<> assets;
    } dirs;
};

struct ice::app::State
{
    ice::Allocator& alloc;

    ice::ProxyAllocator resources_alloc;
    ice::ProxyAllocator modules_alloc;
    ice::ProxyAllocator gamework_alloc;
    ice::ProxyAllocator engine_alloc;

    ice::TaskQueue thread_pool_queue;
    ice::TaskScheduler thread_pool_scheduler;

    ice::UniquePtr<ice::TaskThreadPool> thread_pool;
    ice::UniquePtr<ice::ResourceTracker> resources;
    ice::UniquePtr<ice::ModuleRegister> modules;
    ice::UniquePtr<ice::GameFramework> framework;

    ice::UniquePtr<ice::AssetStorage> assets;
    ice::UniquePtr<ice::WorldTraitArchive> world_traits;
    ice::UniquePtr<ice::Engine> engine;
    ice::UniquePtr<ice::render::RenderDriver> renderer;

    struct ResourceProviders
    {
        ice::UniquePtr<ice::ResourceProvider> filesys;
        ice::UniquePtr<ice::ResourceProvider> modules;
    } providers;

    struct Debug
    {
        ice::UniquePtr<ice::devui::DevUISystem> devui;
    } debug;

    State(ice::Allocator& alloc) noexcept
        : alloc{ alloc }
        , resources_alloc{ alloc, "resources" }
        , modules_alloc{ alloc, "modules" }
        , gamework_alloc{ alloc, "gamework" }
        , engine_alloc{ alloc, "engine" }
        , thread_pool_queue{ }
        , thread_pool_scheduler{ thread_pool_queue }
        , thread_pool{ }
        , resources{ }
        , modules{ ice::create_default_module_register(modules_alloc) }
        , providers{ }
    {
        TaskThreadPoolCreateInfo const pool_info{
            .thread_count = 4,
            .debug_name_format = "ice.thread {}",
        };
        thread_pool = ice::create_thread_pool(alloc, thread_pool_queue, pool_info);

        ResourceTrackerCreateInfo const resource_info{
            .predicted_resource_count = 10000,
            .io_dedicated_threads = 1,
            .flags_io_complete = ice::TaskFlags{ },
            .flags_io_wait = ice::TaskFlags{ }
        };
        resources = ice::create_resource_tracker(resources_alloc, thread_pool_scheduler, resource_info);

        modules->load_module(
            modules_alloc,
            ice::load_log_module,
            ice::unload_log_module
        );
        modules->load_module(
            modules_alloc,
            ice::load_game_module,
            ice::unload_game_module
        );
    }
};

struct ice::app::Runtime
{
    ice::Allocator& alloc;
    ice::ProxyAllocator render_alloc;
    ice::ProxyAllocator runtime_alloc;
    ice::ProxyAllocator app_alloc;

    ice::UniquePtr<ice::platform::WindowSurface> window_surface;
    ice::render::RenderSurface* render_surface; // TODO: Make into a UniquePtr

    ice::UniquePtr<ice::platform::Container> platform_container;

    ice::u32 close_attempts = 0;

    Runtime(ice::Allocator& alloc) noexcept
        : alloc{ alloc }
        , render_alloc{ alloc, "renderer" }
        , runtime_alloc{ alloc, "runtime" }
        , app_alloc{ alloc, "application" }
        , window_surface{ }
        , render_surface{ }
        , platform_container{ }
    { }
};

void ice_init(
    ice::Allocator& alloc,
    ice::app::Factories& factories
) noexcept
{
    using ice::app::Config;
    using ice::app::State;
    using ice::app::Runtime;

    factories.factory_config = [](ice::Allocator& alloc) -> ice::UniquePtr<ice::app::Config> { return ice::make_unique<Config>(&destroy_object<Config>, alloc.create<Config>(alloc)); };
    factories.factory_state = [](ice::Allocator& alloc) -> ice::UniquePtr<ice::app::State> { return ice::make_unique<State>(&destroy_object<State>, alloc.create<State>(alloc)); };
    factories.factory_runtime = [](ice::Allocator& alloc) -> ice::UniquePtr<ice::app::Runtime> { return ice::make_unique<Runtime>(&destroy_object<Runtime>, alloc.create<Runtime>(alloc)); };
}

void ice_args(
    ice::Allocator& alloc,
    ice::app::ArgumentsConfig& arg_config
) noexcept
{
}

auto ice_setup(
    ice::Allocator& alloc,
    ice::app::Arguments const& args,
    ice::app::Config& config,
    ice::app::State& state
) noexcept -> ice::Result
{
    ice::String location = ice::app::location();
    ice::String workingdir = ice::app::workingdir();

    config.dirs.modules = ice::path::directory(ice::app::directory());
    config.dirs.shaders = ice::app::workingdir();
    config.dirs.assets = ice::app::workingdir();

    ice::path::join(config.dirs.shaders, "/obj/VkShaders/GFX-Vulkan-Unoptimized-vk-glslc-1-3/data");
    ice::path::join(config.dirs.assets, "../source/data");
    ice::path::normalize(config.dirs.shaders);
    ice::path::normalize(config.dirs.assets);
    ice::string::push_back(config.dirs.shaders, '/');
    ice::string::push_back(config.dirs.assets, '/');

    ice::String resource_paths[]{ config.dirs.shaders, config.dirs.assets };
    state.providers.filesys = ice::create_resource_provider(state.resources_alloc, resource_paths);
    state.providers.modules = ice::create_resource_provider_dlls(state.resources_alloc, config.dirs.modules);

    state.resources->attach_provider(state.providers.filesys.get());
    state.resources->attach_provider(state.providers.modules.get());
    state.resources->sync_resources();

    state.framework = ice::create_game_object(state.gamework_alloc, *state.resources, *state.modules);
    state.framework->load_modules();

    ice::UniquePtr<ice::AssetTypeArchive> asset_types = ice::create_asset_type_archive(state.engine_alloc);
    ice::load_asset_type_definitions(state.engine_alloc, *state.modules, *asset_types);

    ice::AssetStorageCreateInfo const asset_storage_info{
        .resource_tracker = *state.resources,
        .task_scheduler = state.thread_pool_scheduler,
        .task_flags = ice::TaskFlags{}
    };
    state.assets = ice::create_asset_storage(state.resources_alloc, ice::move(asset_types), asset_storage_info);

    state.world_traits = ice::create_world_trait_archive(state.engine_alloc);
    ice::load_trait_descriptions(state.engine_alloc, *state.modules, *state.world_traits);

    if (ice::build::is_debug || ice::build::is_develop)
    {
        state.debug.devui = ice::devui::create_devui_system(state.engine_alloc, *state.modules);
        state.debug.devui->register_trait(*state.world_traits);
    }

    ice::EngineCreateInfo const create_info{
        .task_scheduler = state.thread_pool_scheduler,
        .asset_storage = *state.assets,
        .trait_archive = *state.world_traits,
        .devui = state.debug.devui.get()
    };

    state.engine = ice::create_engine(state.engine_alloc, *state.modules, create_info);
    state.renderer = ice::render::create_render_driver(alloc, *state.modules);

    if (state.engine == nullptr || state.renderer == nullptr)
    {
        return ice::app::E_FailedApplicationSetup;
    }
    return ice::app::S_ApplicationResume;
}


auto ice_resume(
    ice::app::Config const& config,
    ice::app::State& state,
    ice::app::Runtime& runtime
) noexcept -> ice::Result
{
    // TODO: The whole part starting from framework creation, ending on runner updating needs a good revisit, as it's too complicated as of today.
    if (runtime.window_surface == nullptr)
    {
        runtime.window_surface = ice::platform::create_window_surface(
            runtime.render_alloc,
            state.renderer->render_api()
        );

        // Query surface details and create render surface
        ice::render::SurfaceInfo surface_info;

        [[maybe_unused]]
        bool const query_success = runtime.window_surface->query_details(surface_info);
        ICE_ASSERT(query_success, "Couldn't query surface details from platform surface!");

        runtime.render_surface = state.renderer->create_surface(surface_info);

        using ice::operator|;
        using ice::operator""_sid;

        ice::RenderQueueDefinition queues[]{
            ice::RenderQueueDefinition {
                .name = "default"_sid,
                .flags = ice::render::QueueFlags::Graphics | ice::render::QueueFlags::Present
            },
            ice::RenderQueueDefinition {
                .name = "transfer"_sid,
                .flags = ice::render::QueueFlags::Transfer
            }
        };

        state.framework->startup(*state.engine);

        ice::UniquePtr<ice::gfx::GfxRunner> gfx_runner = state.engine->create_graphics_runner(
            *state.renderer,
            *runtime.render_surface,
            state.framework->graphics_world_template(),
            queues
        );

        if (gfx_runner != nullptr)
        {
            ice::UniquePtr<ice::platform::App> platform_app = state.framework->create_app(ice::move(gfx_runner));
            if (platform_app != nullptr)
            {
                runtime.platform_container = ice::platform::create_app_container(runtime.app_alloc, ice::move(platform_app));
                return ice::app::S_ApplicationUpdate;
            }
        }
    }
    else
    {
        return ice::app::S_ApplicationUpdate;
    }

    return ice::app::E_FailedApplicationResume;
}

auto ice_update(
    ice::app::Config const& config,
    ice::app::State const& state,
    ice::app::Runtime& runtime
) noexcept -> ice::Result
{
    ice::u32 const requested_exit = runtime.platform_container->step();
    if (requested_exit)
    {
        //if (runtime.close_attempts++ < 3)
        //{
        //    return ice::app::S_ApplicationSuspend;
        //}
        //else
        {
            return ice::app::S_ApplicationExit;
        }
    }
    return ice::app::S_ApplicationUpdate;
}

auto ice_suspend(
    ice::app::Config const& config,
    ice::app::State& state,
    ice::app::Runtime& runtime
) noexcept -> ice::Result
{
    runtime.platform_container.reset();
    state.framework->shutdown(*state.engine);
    state.renderer->destroy_surface(runtime.render_surface);
    runtime.window_surface.reset();

    //if (runtime.close_attempts <= 2)
    //{
    //    return ice::app::S_ApplicationResume;
    //}

    state.renderer.reset();
    state.engine.reset();
    state.debug.devui.reset();
    state.framework.reset();
    state.assets.reset();
    state.modules.reset();
    state.resources.reset();

    return ice::app::S_ApplicationExit;
}
