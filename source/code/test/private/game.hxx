/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/framework_app.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/gfx/gfx_graph.hxx>
#include <ice/gfx/gfx_graph_runtime.hxx>

using namespace ice;
using namespace ice::framework;

class TestGame : public Game
{
public:
    TestGame(ice::Allocator& alloc) noexcept;

    void on_setup(ice::framework::State const& state) noexcept override;
    void on_shutdown(ice::framework::State const& state) noexcept override;

    void on_resume(ice::Engine& engine) noexcept override;
    void on_update(ice::Engine& engine, ice::EngineFrame& frame) noexcept override;
    void on_suspend(ice::Engine& engine) noexcept override;

    auto rendergraph(ice::gfx::GfxContext& device) noexcept -> ice::UniquePtr<ice::gfx::GfxGraphRuntime> override;

private:
    ice::Allocator& _allocator;

    ice::UniquePtr<ice::gfx::GfxGraph> _graph;
    ice::UniquePtr<ice::gfx::GfxGraphRuntime> _graph_runtime;

    bool _first_time;
};

#if 0

#include <ice/game_framework.hxx>
#include <ice/game_render_traits.hxx>
#include <ice/game_sprites.hxx>
#include <ice/game_entity.hxx>
#include <ice/game_camera.hxx>
#include <ice/game_tilemap.hxx>
#include <ice/game_physics.hxx>

#include <ice/action/action.hxx>
#include <ice/action/action_system.hxx>
#include <ice/action/action_trigger.hxx>

#include <ice/world/world.hxx>
#include <ice/gfx/gfx_pass.hxx>

#include <ice/mem_allocator_proxy.hxx>
#include <ice/ecs/ecs_data_block.hxx>
#include <ice/ecs/ecs_archetype_index.hxx>

using ice::operator""_sid;
using ice::operator""_uri;

class MyGame : public ice::WorldTrait
{
public:
    static constexpr ice::URI ConfigFile = "file://config.json"_uri;

    MyGame(ice::Allocator& alloc, ice::Clock const& clock) noexcept;

    auto graphics_world_template() const noexcept -> ice::WorldTemplate const&;

    void on_load_modules(ice::GameServices& sercies) noexcept;
    void on_app_startup(ice::Engine& engine) noexcept;
    void on_app_shutdown(ice::Engine& engine) noexcept;

    void on_game_begin(ice::EngineRunner& runner) noexcept;
    void on_game_end() noexcept;

    void on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept;

public:
    ice::ProxyAllocator _allocator;
    ice::Clock const& _clock;

    ice::Engine* _current_engine;

    ice::ecs::ArchetypeIndex _ecs_archetypes;
    ice::ecs::DataBlockPool _ecs_block_pool;
    ice::UniquePtr<ice::ecs::EntityStorage> _ecs_storage;

    ice::UniquePtr<ice::gfx::GfxDynamicPass> _game_gfx_pass;

    ice::UniquePtr<ice::action::ActionTriggerDatabase> _action_triggers{ };
    ice::UniquePtr<ice::action::ActionSystem> _action_system{ };

    bool _active = false;
    ice::World* _test_world;

    char const* _menu = nullptr;
    bool _menu_visible = false;
};

ICE_REGISTER_GAMEAPP(MyGame);

#endif
