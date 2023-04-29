/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/game_framework.hxx>
#include <ice/game_module.hxx>

#include <ice/module_register.hxx>
#include <ice/application.hxx>

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
#include <ice/asset_type_archive.hxx>
#include <ice/asset_storage.hxx>
#include <ice/asset_module.hxx>

#include <ice/log_module.hxx>
#include <ice/log.hxx>
#include <ice/assert.hxx>

#include <ice/mem_allocator_proxy.hxx>

auto game_main(
    ice::Allocator& alloc,
    ice::TaskScheduler& scheduler,
    ice::ResourceTracker& resources
) -> ice::i32
{
    using ice::operator""_uri;
    using ice::operator""_sid;

    ice::i32 main_result = 0;

    ice::ProxyAllocator module_alloc{ alloc, "module-alloc" };

    ice::UniquePtr<ice::ModuleRegister> module_register = ice::create_default_module_register(module_alloc);
    module_register->load_module(
        module_alloc,
        ice::load_log_module,
        ice::unload_log_module
    );

    module_register->load_module(
        module_alloc,
        ice::load_game_module,
        ice::unload_game_module
    );

    {
        ice::ProxyAllocator framework_alloc{ alloc, "game-framework-alloc" };
        ice::ProxyAllocator asset_alloc{ alloc, "asset-alloc" };
        ice::ProxyAllocator engine_alloc{ alloc, "engine-alloc" };
        ice::ProxyAllocator app_alloc{ alloc, "app-alloc" };

        ice::GameFramework* game_framework = ice::create_game_object(
            framework_alloc,
            resources,
            *module_register
        );

        game_framework->load_modules();

        ice::UniquePtr<ice::WorldTraitArchive> trait_archive = ice::create_world_trait_archive(engine_alloc);
        ice::load_trait_descriptions(engine_alloc, *module_register, *trait_archive);

        ice::UniquePtr<ice::AssetTypeArchive> asset_types = ice::create_asset_type_archive(engine_alloc);
        ice::load_asset_type_definitions(asset_alloc, *module_register, *asset_types);

        ice::AssetStorageCreateInfo const asset_storage_info{
            .resource_tracker = resources,
            .task_scheduler = scheduler,
            .task_flags = ice::TaskFlags{}
        };
        ice::UniquePtr<ice::AssetStorage> asset_storage = ice::create_asset_storage(asset_alloc, ice::move(asset_types), asset_storage_info);

        ice::UniquePtr<ice::devui::DevUISystem> engine_devui{ };
        if (ice::build::is_debug || ice::build::is_develop)
        {
            engine_devui = ice::devui::create_devui_system(engine_alloc, *module_register);
            engine_devui->register_trait(*trait_archive);
        }

        ice::EngineCreateInfo const create_info{
            .task_scheduler = scheduler,
            .asset_storage = *asset_storage,
            .trait_archive = *trait_archive,
            .devui = engine_devui.get()
        };

        ice::UniquePtr<ice::Engine> engine = ice::create_engine(engine_alloc, *module_register, create_info);
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

            using ice::operator|;

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

            game_framework->startup(*engine);

            ice::UniquePtr<ice::gfx::GfxRunner> gfx_runner = engine->create_graphics_runner(
                *render_driver,
                *render_surface,
                game_framework->graphics_world_template(),
                queues
            );

            if (gfx_runner != nullptr)
            {

                ice::UniquePtr<ice::platform::App> platform_app = game_framework->create_app(ice::move(gfx_runner));
                if (platform_app != nullptr)
                {
                    main_result = ice::platform::create_app_container(app_alloc, ice::move(platform_app))->run();
                }

            }

            game_framework->shutdown(*engine);

            render_driver->destroy_surface(render_surface);
            app_window = nullptr;
        }

        render_driver = nullptr;
        engine = nullptr;
        engine_devui = nullptr;

        framework_alloc.destroy(game_framework);

        asset_storage = nullptr;
        module_register = nullptr;
    }

    return main_result;
}
