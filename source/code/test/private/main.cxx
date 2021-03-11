#include <ice/allocator.hxx>
#include <ice/module_register.hxx>
#include <ice/resource.hxx>
#include <ice/resource_query.hxx>
#include <ice/resource_system.hxx>
#include <ice/asset_system.hxx>
#include <ice/asset_module.hxx>
#include <ice/render/render_module.hxx>
#include <ice/engine_module.hxx>
#include <ice/log.hxx>
#include <ice/log_module.hxx>
#include <ice/assert.hxx>

#include "game_app.hxx"

using ice::operator""_sid;
using ice::operator""_uri;

ice::i32 game_main(ice::Allocator& alloc, ice::ResourceSystem& resource_system)
{
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

    ice::UniquePtr<ice::Engine> engine = ice::create_engine(alloc, *asset_system, *module_register);
    if (engine != nullptr)
    {
        ice::UniquePtr<ice::render::RenderDriver> render_driver = ice::render::create_render_driver(
            alloc, *module_register
        );

        ice::UniquePtr<ice::platform::Container> app = ice::platform::create_app_container(
            alloc,
            ice::make_unique<ice::platform::App, TestGameApp>(
                alloc,
                alloc,
                *engine,
                *render_driver
            )
        );

        app->run();
    }

    engine = nullptr;
    asset_system = nullptr;

    return 0;
}
