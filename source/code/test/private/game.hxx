#pragma once
#include <ice/allocator.hxx>
#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_stage.hxx>
#include <ice/gfx/gfx_pass.hxx>
#include <ice/entity/entity_storage.hxx>
#include <ice/world/world_manager.hxx>
#include <ice/world/world.hxx>
#include <ice/engine.hxx>

#include <ice/task.hxx>

#include "systems/terrain.hxx"
#include "systems/camera.hxx"
#include "systems/imgui.hxx"

#include <ice/game_framework.hxx>
#include <ice/module_register.hxx>
#include <ice/resource_system.hxx>
#include <ice/resource.hxx>

using ice::operator""_sid;
using ice::operator""_uri;

class MyGame
{
public:
    static constexpr ice::URI ConfigFile = "file://../source/data/config.json"_uri;

    MyGame(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _archetype_alloc{ _allocator }
        , _archetype_index{ ice::create_archetype_index(_allocator) }
        , _entity_storage{ _allocator, *_archetype_index, _archetype_alloc  }
    {
    }

    void on_load_modules(ice::GameServices& sercies) noexcept
    {
        ice::ModuleRegister& mod = sercies.module_registry();
        ice::ResourceSystem& res = sercies.resource_system();

        ice::Resource* const pipelines_module = res.request("res://iceshard_pipelines.dll"_uri);
        ice::Resource* const engine_module = res.request("res://iceshard.dll"_uri);
        ice::Resource* const vulkan_module = res.request("res://vulkan_renderer.dll"_uri);

        ICE_ASSERT(pipelines_module != nullptr, "Missing `iceshard_pipelines.dll` module!");
        ICE_ASSERT(engine_module != nullptr, "Missing `iceshard.dll` module!");
        ICE_ASSERT(vulkan_module != nullptr, "Missing `vulkan_renderer.dll` module!");

        mod.load_module(_allocator, pipelines_module->location().path);
        mod.load_module(_allocator, engine_module->location().path);
        mod.load_module(_allocator, vulkan_module->location().path);
    }

    void on_app_startup(ice::Engine& engine) noexcept
    {
        _current_engine = &engine;
        ICE_LOG(
            ice::LogSeverity::Debug, ice::LogTag::Game,
            "Hello, world!"
        );

        ice::WorldManager& world_manager = _current_engine->world_manager();
        ice::World* world = world_manager.create_world("default"_sid, &_entity_storage);
        add_world_traits(engine, world);
    }

    void on_app_shutdown(ice::Engine& engine) noexcept
    {
        ice::WorldManager& world_manager = _current_engine->world_manager();
        remove_world_traits(world_manager.find_world("default"_sid));
        world_manager.destroy_world("default"_sid);

        ICE_LOG(
            ice::LogSeverity::Debug, ice::LogTag::Game,
            "Goodbye, world!"
        );
        _current_engine = nullptr;;
    }

    void on_game_begin(ice::EngineRunner& runner) noexcept;

    void on_game_end() noexcept;

protected:
    struct TraitContainer;
    TraitContainer* _traits = nullptr;
    void add_world_traits(
        ice::Engine& engine,
        ice::World* world
    ) noexcept;
    void remove_world_traits(ice::World* world) noexcept;

public:
    ice::Allocator& _allocator;
    ice::Engine* _current_engine;

    ice::ArchetypeBlockAllocator _archetype_alloc;
    ice::UniquePtr<ice::ArchetypeIndex> _archetype_index;
    ice::EntityStorage _entity_storage;
};

ICE_REGISTER_GAMEAPP(MyGame);
