#include "game.hxx"

#include <ice/game_actor.hxx>
#include <ice/game_anim.hxx>
#include <ice/game_physics.hxx>

#include <ice/engine.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_devui.hxx>
#include <ice/engine_shards.hxx>
#include <ice/devui/devui_render_trait.hxx>

#include <ice/entity/entity_index.hxx>
#include <ice/entity/entity_storage.hxx>
#include <ice/entity/entity_command_buffer.hxx>
#include <ice/entity/entity_query_utils.hxx>

#include <ice/archetype/archetype_index.hxx>
#include <ice/archetype/archetype_block_allocator.hxx>

#include <ice/world/world.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/world/world_manager.hxx>

#include <ice/gfx/gfx_pass.hxx>
#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_frame.hxx>
#include <ice/gfx/gfx_pass.hxx>
#include <ice/gfx/gfx_runner.hxx>
#include <ice/render/render_swapchain.hxx>

#include <ice/input/input_event.hxx>
#include <ice/input/input_keyboard.hxx>
#include <ice/input/input_tracker.hxx>

#include <ice/task_thread_pool.hxx>
#include <ice/module_register.hxx>
#include <ice/resource_system.hxx>
#include <ice/asset_system.hxx>
#include <ice/asset_pipeline.hxx>
#include <ice/resource.hxx>
#include <ice/assert.hxx>
#include <ice/shard.hxx>

#include <ice/ecs/ecs_archetype_index.hxx>
#include <ice/ecs/ecs_entity_index.hxx>
#include <ice/ecs/ecs_entity_storage.hxx>
#include <ice/ecs/ecs_entity_operations.hxx>


MyGame::MyGame(ice::Allocator& alloc, ice::Clock const& clock) noexcept
    : _allocator{ alloc, "MyGame-alloc" }
    , _clock{ clock }
    , _current_engine{ nullptr }
    , _archetype_alloc{ _allocator }
    , _archetype_index{ ice::create_archetype_index(_allocator) }
    , _entity_storage{ _allocator, *_archetype_index, _archetype_alloc }
    , _game_gfx_pass{ ice::gfx::create_dynamic_pass(_allocator) }
    , _test_world{ nullptr }
    , _render_world{ nullptr }
{
}

void MyGame::on_load_modules(ice::GameServices& sercies) noexcept
{
    ice::ModuleRegister& mod = sercies.module_registry();
    ice::ResourceSystem& res = sercies.resource_system();

    ice::Resource* const pipelines_module = res.request("res://iceshard_pipelines.dll"_uri);
    ice::Resource* const engine_module = res.request("res://iceshard.dll"_uri);
    ice::Resource* const vulkan_module = res.request("res://vulkan_renderer.dll"_uri);
    ice::Resource* const imgui_module = res.request("res://imgui_module.dll"_uri);

    ICE_ASSERT(pipelines_module != nullptr, "Missing `iceshard_pipelines.dll` module!");
    ICE_ASSERT(engine_module != nullptr, "Missing `iceshard.dll` module!");
    ICE_ASSERT(vulkan_module != nullptr, "Missing `vulkan_renderer.dll` module!");

    mod.load_module(_allocator, pipelines_module->location().path);
    mod.load_module(_allocator, engine_module->location().path);
    mod.load_module(_allocator, vulkan_module->location().path);

    ice::register_asset_modules(_allocator, mod);

    if (imgui_module != nullptr)
    {
        mod.load_module(_allocator, imgui_module->location().path);
    }
}

void MyGame::on_app_startup(ice::Engine& engine, ice::gfx::GfxRunner& gfx_runner) noexcept
{
    _current_engine = &engine;
    ICE_LOG(
        ice::LogSeverity::Debug, ice::LogTag::Game,
        "Hello, world!"
    );

    {
        ice::Allocator& alloc{ _allocator };

        ice::ecs::EntityIndex entity_index{ alloc, 1000 };
        ice::ecs::ArchetypeIndex archetype_index{ alloc };
        ice::ecs::EntityOperations entity_operations{ alloc };

        ice::ecs::Entity entities[10];
        entity_index.create_many(entities);
        {
            auto a1 = archetype_index.register_archetype(ice::ecs::static_validation::Validation_Archetype_1);
            auto a2 = archetype_index.register_archetype(ice::ecs::static_validation::Validation_Archetype_2);

            ice::ecs::EntityStorage entity_storage{ alloc, archetype_index };
            {
                ice::ecs::queue_set_archetype(entity_operations, entities, a1, true);

                ice::ShardContainer shards{ _allocator };
                entity_storage.execute_operations(entity_operations, shards);
                entity_operations.clear();

                ice::ecs::queue_remove_entity(entity_operations, ice::shard_shatter<ice::ecs::EntityHandle>(shards._data[3]));
                ice::ecs::queue_set_archetype(entity_operations, entities[3], a1);
                entity_storage.execute_operations(entity_operations, shards);
            }
        }
        entity_index.destroy_many(entities);
    }

    ice::EngineDevUI& devui = engine.developer_ui();

    ice::Asset test_tilemap = engine.asset_system().request(ice::AssetType::TileMap, "/cotm/test_level_2/tiled/0002_Level_1"_sid);
    ICE_ASSERT(test_tilemap != ice::Asset::Invalid, "");

    ice::Data tilemap_data;
    ice::asset_data(test_tilemap, tilemap_data);

    ice::TileMap const* tilemap = reinterpret_cast<ice::TileMap const*>(tilemap_data.location);

    _trait_physics = ice::create_trait_physics(_allocator);
    _trait_tilemap = ice::create_tilemap_trait(_allocator, *_trait_physics);
    _trait_tilemap->load_tilemap(*tilemap);

    _trait_animator = ice::create_trait_animator(_allocator);
    _trait_actor = ice::create_trait_actor(_allocator);
    _trait_render_gfx = ice::create_trait_render_gfx(_allocator);
    _trait_render_clear = ice::create_trait_render_clear(_allocator, "ice.gfx.stage.clear"_sid);
    _trait_render_finish = ice::create_trait_render_finish(_allocator, "ice.gfx.stage.finish"_sid);
    _trait_render_postprocess = ice::create_trait_render_postprocess(_allocator, "ice.gfx.stage.postprocess"_sid);
    _trait_render_sprites = ice::create_trait_render_sprites(_allocator, "ice.gfx.stage.sprites"_sid);
    _trait_render_tilemap = ice::create_trait_render_tilemap(_allocator, "ice.gfx.stage.tilemap"_sid);
    _trait_render_debug = ice::create_trait_render_debug(_allocator, "ice.gfx.stage.debug_render"_sid);
    _trait_render_camera = ice::create_trait_camera(_allocator);

    gfx_runner.add_trait("ice.render_camera"_sid, _trait_render_camera.get());
    gfx_runner.add_trait("ice.render_gfx"_sid, _trait_render_gfx.get());
    gfx_runner.add_trait("ice.render_clear"_sid, _trait_render_clear.get());
    gfx_runner.add_trait("ice.render_postprocess"_sid, _trait_render_postprocess.get());
    gfx_runner.add_trait("ice.render_finish"_sid, _trait_render_finish.get());
    gfx_runner.add_trait("ice.render_tilemap"_sid, _trait_render_tilemap.get());
    gfx_runner.add_trait("ice.render_sprites"_sid, _trait_render_sprites.get());
    gfx_runner.add_trait("ice.render_debug"_sid, _trait_render_debug.get());


    _game_gfx_pass->add_stage("ice.gfx.stage.clear"_sid);
    _game_gfx_pass->add_stage("ice.gfx.stage.tilemap"_sid, "ice.gfx.stage.clear"_sid);
    _game_gfx_pass->add_stage("ice.gfx.stage.sprites"_sid, "ice.gfx.stage.tilemap"_sid, "ice.gfx.stage.clear"_sid);
    _game_gfx_pass->add_stage("ice.gfx.stage.postprocess"_sid, "ice.gfx.stage.sprites"_sid);
    _game_gfx_pass->add_stage("ice.gfx.stage.debug_render"_sid, "ice.gfx.stage.postprocess"_sid);

    if (devui.world_trait() != nullptr)
    {
        gfx_runner.add_trait("ice.devui"_sid, devui.world_trait());

        _game_gfx_pass->add_stage(devui.world_trait()->gfx_stage_name(), "ice.gfx.stage.postprocess"_sid);
        _game_gfx_pass->add_stage("ice.gfx.stage.finish"_sid, devui.world_trait()->gfx_stage_name());
    }
    else
    {
        _game_gfx_pass->add_stage("ice.gfx.stage.finish"_sid, "ice.gfx.stage.postprocess"_sid);
    }

    ice::WorldManager& world_manager = engine.world_manager();
    _render_world = world_manager.create_world(GraphicsWorldName, &_entity_storage);

    _test_world = world_manager.create_world("game.test_world"_sid, &_entity_storage);
    _test_world->add_trait("ice.tilemap"_sid, _trait_tilemap.get());
    _test_world->add_trait("ice.physics"_sid, _trait_physics.get());
    _test_world->add_trait("ice.anim"_sid, _trait_animator.get());
    _test_world->add_trait("ice.actor"_sid, _trait_actor.get());

    _test_world->add_trait("game"_sid, this);

    ice::ArchetypeHandle ortho_arch = _archetype_index->register_archetype<ice::Camera, ice::CameraOrtho>(&_archetype_alloc);
    ice::ArchetypeHandle persp_arch = _archetype_index->register_archetype<ice::Camera, ice::CameraPerspective>(&_archetype_alloc);
    _archetype_index->register_archetype<ice::Transform2DStatic, ice::Sprite>(&_archetype_alloc);
    _archetype_index->register_archetype<ice::Transform2DDynamic, ice::Sprite, ice::SpriteTile, ice::Animation, ice::AnimationState, ice::PhysicsBody>(&_archetype_alloc);
    _archetype_index->register_archetype<ice::Transform2DDynamic, ice::Sprite, ice::SpriteTile, ice::Animation, ice::AnimationState, ice::Actor, ice::PhysicsVelocity, ice::PhysicsBody>(&_archetype_alloc);

    _action_triggers = ice::action::create_trigger_database(_allocator);
    _action_system = ice::action::create_action_system(_allocator, _clock, *_action_triggers);
    ice::action::setup_common_triggers(*_action_triggers);

    using ice::operator""_shard;
    using ice::operator""_shardid;
    using namespace ice::action;

    Action my_action;
    my_action.name = "test-action"_sid;
    my_action.stage_count = 1;
    my_action.trigger_count = 3;

    ActionStage stages[]{
        ActionStage
        {
            .stage_shardid = "event/player/can_jump_again"_shardid,
            .success_trigger_offset = 0,
            .success_trigger_count = 1,
            .failure_trigger_offset = 1,
            .failure_trigger_count = 1,
            .reset_trigger_offset = 2
        }
    };

    ice::input::InputEvent event_w{ };
    ice::input::InputEvent event_a{ };
    ice::input::InputEvent event_d{ };

    event_w.device = ice::input::make_device_handle(ice::input::DeviceType::Keyboard, ice::input::DeviceIndex{ 0 });
    event_a.device = ice::input::make_device_handle(ice::input::DeviceType::Keyboard, ice::input::DeviceIndex{ 0 });
    event_d.device = ice::input::make_device_handle(ice::input::DeviceType::Keyboard, ice::input::DeviceIndex{ 0 });

    event_w.identifier = ice::input::input_identifier(ice::input::DeviceType::Keyboard, ice::input::KeyboardKey::KeyW, ice::input::key_identifier_base_value);
    event_a.identifier = ice::input::input_identifier(ice::input::DeviceType::Keyboard, ice::input::KeyboardKey::KeyA, ice::input::key_identifier_base_value);
    event_d.identifier = ice::input::input_identifier(ice::input::DeviceType::Keyboard, ice::input::KeyboardKey::KeyD, ice::input::key_identifier_base_value);

    event_w.value_type = ice::input::InputValueType::Button;
    event_a.value_type = ice::input::InputValueType::Button;
    event_d.value_type = ice::input::InputValueType::Button;

    event_w.value.button.state.pressed = true;
    event_a.value.button.state.pressed = true;
    event_d.value.button.state.pressed = true;

    ActionTrigger triggers[]{
        ActionTrigger
        {
            .name = "trigger.action-input-button"_sid,
            .user_shard = "key"_shard | event_w
        },
        ActionTrigger
        {
            .name = "trigger.action-input-button"_sid,
            .user_shard = "key"_shard | event_a
        },
        ActionTrigger
        {
            .name = "trigger.elapsed-time"_sid,
            .user_shard = "time"_shard | ice::f32{ 3.f }
        }
    };

    my_action.stages = stages;
    my_action.triggers = triggers;
    _action_system->create_action(my_action.name, my_action);
}

void MyGame::on_app_shutdown(ice::Engine& engine) noexcept
{
    _action_system = nullptr;
    _action_triggers = nullptr;

    _test_world->remove_trait("game"_sid);

    _test_world->remove_trait("ice.actor"_sid);
    _test_world->remove_trait("ice.anim"_sid);
    _test_world->remove_trait("ice.tilemap"_sid);
    _test_world->remove_trait("ice.physics"_sid);

    ice::WorldManager& world_manager = engine.world_manager();
    world_manager.destroy_world("game.test_world"_sid);
    world_manager.destroy_world(GraphicsWorldName);

    _trait_render_camera = nullptr;
    _trait_render_debug = nullptr;
    _trait_render_postprocess = nullptr;
    _trait_render_tilemap = nullptr;
    _trait_render_sprites = nullptr;
    _trait_render_finish = nullptr;
    _trait_render_clear = nullptr;
    _trait_render_gfx = nullptr;
    _trait_actor = nullptr;
    _trait_animator = nullptr;
    _trait_tilemap = nullptr;
    _trait_physics = nullptr;
    _game_gfx_pass = nullptr;

    ICE_LOG(
        ice::LogSeverity::Debug, ice::LogTag::Game,
        "Goodbye, world!"
    );
    _current_engine = nullptr;
}

void MyGame::on_game_begin(ice::EngineRunner& runner) noexcept
{
    using ice::operator""_sid_hash;

    ice::vec2u extent = runner.graphics_device().swapchain().extent();

    constexpr ice::StringID components[]{ ice::Camera::Identifier, ice::CameraOrtho::Identifier };
    constexpr ice::ArchetypeQueryCriteria query_criteria{
        .components = components
    };

    constexpr ice::StringID sprite_components[]{
        ice::Transform2DDynamic::Identifier, ice::Sprite::Identifier, ice::SpriteTile::Identifier, ice::Animation::Identifier, ice::AnimationState::Identifier
    };
    constexpr ice::ArchetypeQueryCriteria sprite_query_criteria{
        .components = sprite_components
    };
    constexpr ice::StringID actor_components[]{
        ice::Transform2DDynamic::Identifier, ice::Sprite::Identifier, ice::SpriteTile::Identifier, ice::Animation::Identifier, ice::AnimationState::Identifier, ice::Actor::Identifier
    };
    constexpr ice::ArchetypeQueryCriteria actor_query_criteria{
        .components = actor_components
    };

    ice::ArchetypeHandle ortho_arch = _archetype_index->find_archetype(query_criteria);
    ice::ArchetypeHandle sprite_arch = _archetype_index->find_archetype(sprite_query_criteria);
    ice::ArchetypeHandle actor_arch = _archetype_index->find_archetype(actor_query_criteria);

    ice::Entity camera_entity = _current_engine->entity_index().create();
    ice::Entity sprite_entity = _current_engine->entity_index().create();
    ice::Entity sprite_entity2 = _current_engine->entity_index().create();
    ice::Entity sprite_entity3 = _current_engine->entity_index().create();

    ice::Camera const camera{
        .name = "camera.default"_sid,
        .position = { 0.f, 0.f, 0.f },
        .front = { 0.f, 0.f, -1.f }
    };
    ice::CameraOrtho const orto_values{
        .left_right = { 0.f, (ice::f32) extent.x / 2.f },
        .bottom_top = { 0.f, (ice::f32) extent.y / 2.f },
        .near_far = { 0.1f, 100.f }
    };
    _entity_storage.set_archetype_with_data(camera_entity, ortho_arch, camera, orto_values);

    ice::Animation anim{
        .animation = "cotm_idle"_sid_hash,
        .speed = 1.f / 60.f
    };
    ice::Transform2DDynamic sprite_pos{
        .position = { 48.f, 448.f, -1.f },
        .scale = { 1.f, 0.f }
    };
    ice::Sprite sprite{
        .material = "/cotm/cotm_hero"_sid,
    };
    ice::SpriteTile sprite_tile{
        .material_tile = { 0, 0 }
    };
    _entity_storage.set_archetype_with_data(sprite_entity, sprite_arch, anim, sprite_pos, sprite, sprite_tile, ice::PhysicsBody{ .shape = ice::PhysicsShape::Capsule, .dimensions = { 16.f, 32.f } });

    sprite_pos.position = { 48.f * 2, 448.f, -1.f };
    sprite_tile.material_tile = { 0, 1 };
    anim.speed = 1.f / 15.f;
    ice::Actor actor{ .type = ice::ActorType::Player };
    _entity_storage.set_archetype_with_data(sprite_entity2, actor_arch, anim, sprite_pos, sprite, sprite_tile, actor, ice::PhysicsBody{ .shape = ice::PhysicsShape::Capsule, .dimensions = { 16.f, 32.f } });

    sprite_pos.position = { 48.f * 3, 448.f, -1.f };
    sprite_tile.material_tile = { 4, 5 };
    anim.speed = 1.f / 30.f;
    sprite.material = "/cotm/tileset_a"_sid;
    anim.animation = "null"_sid_hash;
    _entity_storage.set_archetype_with_data(sprite_entity3, sprite_arch, anim, sprite_pos, sprite, sprite_tile, ice::PhysicsBody{ .dimensions = { 16.f, 16.f } });

    ice::math::deg d1{ 180 };
    ice::math::rad d1r = radians(d1);

    ice::Shard shards[]{
        ice::Shard_WorldActivate | _test_world,
        ice::Shard_WorldActivate | _render_world,
        ice::Shard_SetDefaultCamera | "camera.default"_sid_hash
    };
    ice::shards::push_back(
        runner.current_frame().shards(),
        shards
    );
}

void MyGame::on_game_end() noexcept
{
}

void MyGame::on_update(ice::EngineFrame& frame, ice::EngineRunner& runner, ice::WorldPortal& portal) noexcept
{
    runner.graphics_frame().enqueue_pass("default"_sid, "game.render"_sid, _game_gfx_pass.get());

    bool was_active = _active;
    for (ice::input::InputEvent const& event : frame.input_events())
    {
        if (ice::input::input_identifier(ice::input::DeviceType::Keyboard, ice::input::KeyboardKey::KeyP) == event.identifier)
        {
            if (event.value.button.state.clicked)
            {
                _active = !_active;
            }
        }
    }

    _action_system->step_actions(frame.shards());

    using ice::operator""_shardid;
    for (ice::Shard const shard : frame.shards())
    {
        if (shard == "event/player/can_jump_again"_shardid)
        {
            ICE_LOG(ice::LogSeverity::Debug, ice::LogTag::Game, "We can jump!");
        }
        if (shard == ice::action::Shard_ActionEventSuccess)
        {
            ICE_LOG(ice::LogSeverity::Debug, ice::LogTag::Game, "Success!");
        }
        if (shard == ice::action::Shard_ActionEventFailed)
        {
            ICE_LOG(ice::LogSeverity::Debug, ice::LogTag::Game, "Failure!");
        }
        if (shard == ice::action::Shard_ActionEventReset)
        {
            ICE_LOG(ice::LogSeverity::Debug, ice::LogTag::Game, "Reset!");
        }
    }


    if (was_active == true && _active == false)
    {
        ice::shards::push_back(
            runner.current_frame().shards(),
            ice::Shard_WorldDeactivate | _test_world
        );
    }
    else if (was_active == false && _active == true)
    {
        ice::shards::push_back(
            runner.current_frame().shards(),
            ice::Shard_WorldActivate | _test_world
        );
    }
}
