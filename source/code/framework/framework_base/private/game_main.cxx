#include <ice/game_framework.hxx>
#include <ice/module_register.hxx>
#include <ice/application.hxx>

#include <ice/platform_app.hxx>
#include <ice/platform_window_surface.hxx>

#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_runner.hxx>

#include <ice/render/render_module.hxx>
#include <ice/engine.hxx>
#include <ice/engine_module.hxx>

#include <ice/devui/devui_module.hxx>
#include <ice/devui/devui_system.hxx>

#include <ice/resource_query.hxx>
#include <ice/resource_system.hxx>
#include <ice/asset_system.hxx>
#include <ice/asset_module.hxx>

#include <ice/log_module.hxx>
#include <ice/log.hxx>
#include <ice/assert.hxx>

#include <ice/memory/memory_globals.hxx>
#include <ice/memory/proxy_allocator.hxx>

auto game_main(ice::Allocator& alloc, ice::ResourceSystem& resources) -> ice::i32
{
    using ice::operator""_uri;
    using ice::operator""_sid;

    ice::i32 main_result = 0;

    ice::memory::ProxyAllocator module_alloc{ alloc, "module-alloc" };

    ice::UniquePtr<ice::ModuleRegister> module_register = ice::create_default_module_register(module_alloc);
    module_register->load_module(
        module_alloc,
        ice::load_log_module,
        ice::unload_log_module
    );

    {
        ice::memory::ProxyAllocator framework_alloc{ alloc, "game-framework-alloc" };
        ice::memory::ProxyAllocator asset_alloc{ alloc, "asset-alloc" };
        ice::memory::ProxyAllocator engine_alloc{ alloc, "engine-alloc" };
        ice::memory::ProxyAllocator app_alloc{ alloc, "app-alloc" };

        ice::GameFramework* game_framework = ice::create_game_object(
            framework_alloc,
            resources,
            *module_register
        );

        game_framework->load_modules();

        // Mount the current working directory
        //  This allows to automatically detect any `config.json` file stored with the executable.
        resources.mount("dir://."_uri);

        ice::URI const config_file = game_framework->config_uri();
        ice::u32 const mounted_files = resources.mount(config_file);
        ICE_ASSERT(
            mounted_files >= 1,
            "Missing config file for URI: {}",
            config_file.path
        );

        ice::UniquePtr<ice::AssetSystem> asset_system = ice::make_unique_null<ice::AssetSystem>();

        {
            ice::ResourceQuery resource_query;
            resources.query_changes(resource_query);
            resources.mount("file://mount.isr"_uri);

            ice::Resource* const mount_file = resources.request("urn://mount.isr"_uri);
            if (mount_file != nullptr)
            {
                ICE_LOG(ice::LogSeverity::Info, ice::LogTag::Game, "Custom mount file found: {}\n", mount_file->location().path);
            }

            resources.query_changes(resource_query);
            resources.mount("dir://../source/data"_uri);
            resources.query_changes(resource_query);

            asset_system = ice::create_asset_system(asset_alloc, resources);
            ice::load_asset_pipeline_modules(asset_alloc, *module_register, *asset_system);
            asset_system->bind_resources(resource_query.objects);
        }

        ice::UniquePtr<ice::devui::DevUISystem> engine_devui = ice::devui::create_devui_system(engine_alloc, *module_register);
        ice::UniquePtr<ice::Engine> engine = ice::create_engine(engine_alloc, *asset_system, *module_register, engine_devui.get());
        ice::UniquePtr<ice::render::RenderDriver> render_driver = ice::render::create_render_driver(alloc, *module_register);

        if (engine != nullptr && render_driver != nullptr)
        {
            // TODO: Refactor the platform and render surface creation and lifetime handling.
            // Currently it's far from ideal and for now we assume they will always succede.

            // Create app surface
            ice::UniquePtr<ice::platform::WindowSurface> app_window = ice::platform::create_window_surface(
                alloc, render_driver->render_api()
            );

            // Query surface details and create render surface
            ice::render::SurfaceInfo surface_info;

            [[maybe_unused]]
            bool const query_success = app_window->query_details(surface_info);
            ICE_ASSERT(query_success, "Couldn't query surface details from platform surface!");

            ice::render::RenderSurface* render_surface = render_driver->create_surface(surface_info);

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

            ice::UniquePtr<ice::gfx::GfxRunner> gfx_runner = engine->create_graphics_runner(
                *render_driver,
                *render_surface,
                queues
            );

            if (gfx_runner != nullptr)
            {
                gfx_runner->set_graphics_world(game_framework->graphics_world_name());
                game_framework->startup(*engine);

                ice::UniquePtr<ice::platform::App> platform_app = game_framework->create_app(ice::move(gfx_runner));
                if (platform_app != nullptr)
                {
                    main_result = ice::platform::create_app_container(app_alloc, ice::move(platform_app))->run();
                }

                game_framework->shutdown(*engine);
            }

            render_driver->destroy_surface(render_surface);
            app_window = nullptr;
        }

        render_driver = nullptr;
        engine = nullptr;
        engine_devui = nullptr;

        framework_alloc.destroy(game_framework);

        asset_system = nullptr;
        module_register = nullptr;
    }

    return main_result;
}
