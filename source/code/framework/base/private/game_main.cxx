#include <ice/game_framework.hxx>
#include <ice/module_register.hxx>
#include <ice/application.hxx>

#include <ice/platform_app.hxx>

#include <ice/render/render_module.hxx>
#include <ice/engine_module.hxx>

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

    ice::i32 main_result = 0;

    ice::UniquePtr<ice::ModuleRegister> module_register = ice::create_default_module_register(alloc);
    module_register->load_module(
        alloc,
        ice::load_log_module,
        ice::unload_log_module
    );

    ice::GameFramework* game_framework = ice::create_game_object(
        alloc,
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

    ice::UniquePtr<ice::AssetSystem> asset_system = ice::create_asset_system(alloc, resources);
    ice::load_asset_pipeline_modules(alloc, *module_register, *asset_system);
    asset_system->bind_resources(resource_query.objects);

    ice::UniquePtr<ice::Engine> engine = ice::create_engine(alloc, *asset_system, *module_register);
    if (engine != nullptr)
    {
        game_framework->startup(*engine);

        ice::UniquePtr<ice::platform::App> platform_app = game_framework->platform_app();
        if (platform_app != nullptr)
        {
            main_result = ice::platform::create_app_container(alloc, ice::move(platform_app))->run();
        }

        game_framework->shutdown(*engine);
    }
    engine = nullptr;

    alloc.destroy(game_framework);

    asset_system = nullptr;
    module_register = nullptr;

    return main_result;
}
