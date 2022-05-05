#pragma once
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

#include <ice/memory/proxy_allocator.hxx>
#include <ice/ecs/ecs_data_block.hxx>
#include <ice/ecs/ecs_archetype_index.hxx>

using ice::operator""_sid;
using ice::operator""_uri;

class MyGame : public ice::WorldTrait
{
public:
    static constexpr ice::URI ConfigFile = ice::URI{ ice::scheme_file, u8"config.json" };

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
    ice::memory::ProxyAllocator _allocator;
    ice::Clock const& _clock;

    ice::Engine* _current_engine;

    ice::ecs::ArchetypeIndex _ecs_archetypes;
    ice::ecs::DataBlockPool _ecs_block_pool;
    ice::UniquePtr<ice::ecs::EntityStorage> _ecs_storage;

    ice::UniquePtr<ice::gfx::GfxDynamicPass> _game_gfx_pass;

    ice::UniquePtr<ice::action::ActionTriggerDatabase> _action_triggers{ ice::make_unique_null<ice::action::ActionTriggerDatabase>() };
    ice::UniquePtr<ice::action::ActionSystem> _action_system{ ice::make_unique_null<ice::action::ActionSystem>() };

    bool _active = false;
    ice::World* _test_world;
};

ICE_REGISTER_GAMEAPP(MyGame);
