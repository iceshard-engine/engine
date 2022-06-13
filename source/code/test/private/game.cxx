#include "game.hxx"

#include <ice/game_actor.hxx>
#include <ice/game_anim.hxx>
#include <ice/game_physics.hxx>
#include <ice/game_tilemap.hxx>
#include <ice/game_ui.hxx>

#include <ice/engine.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_devui.hxx>
#include <ice/engine_shards.hxx>
#include <ice/devui/devui_render_trait.hxx>

#include <ice/world/world.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/world/world_portal.hxx>
#include <ice/world/world_manager.hxx>
#include <ice/world/world_assembly.hxx>

#include <ice/gfx/gfx_pass.hxx>
#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_frame.hxx>
#include <ice/gfx/gfx_pass.hxx>
#include <ice/gfx/gfx_runner.hxx>
#include <ice/gfx/gfx_font.hxx>
#include <ice/render/render_image.hxx>
#include <ice/render/render_swapchain.hxx>

#include <ice/input/input_event.hxx>
#include <ice/input/input_keyboard.hxx>
#include <ice/input/input_tracker.hxx>

#include <ice/task_sync_wait.hxx>
#include <ice/task_thread_pool.hxx>
#include <ice/module_register.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/resource_provider.hxx>
#include <ice/asset_storage.hxx>
#include <ice/resource.hxx>
#include <ice/assert.hxx>
#include <ice/shard.hxx>

#include <ice/ui_asset.hxx>
#include <ice/ui_data.hxx>

#include <ice/ecs/ecs_archetype_index.hxx>
#include <ice/ecs/ecs_entity_index.hxx>
#include <ice/ecs/ecs_entity_storage.hxx>
#include <ice/ecs/ecs_entity_operations.hxx>


MyGame::MyGame(ice::Allocator& alloc, ice::Clock const& clock) noexcept
    : _allocator{ alloc, "MyGame-alloc" }
    , _clock{ clock }
    , _current_engine{ nullptr }
    , _ecs_archetypes{ _allocator }
    , _ecs_block_pool{ _allocator }
    , _ecs_storage{ ice::make_unique_null<ice::ecs::EntityStorage>() }
    , _game_gfx_pass{ ice::gfx::create_dynamic_pass(_allocator) }
    , _test_world{ nullptr }
{
}

auto MyGame::graphics_world_template() const noexcept -> ice::WorldTemplate const&
{
    if constexpr (ice::build::is_debug || ice::build::is_develop)
    {
        static ice::StringID constexpr graphics_traits[]{
            ice::Constant_TraitName_RenderCamera,
            ice::Constant_TraitName_RenderBase,
            ice::Constant_TraitName_RenderTextureLoader,
            ice::Constant_TraitName_RenderClear,
            ice::Constant_TraitName_RenderSprites,
            ice::Constant_TraitName_RenderTilemap,
            ice::Constant_TraitName_RenderGlyphs,
            ice::Constant_TraitName_RenderPostprocess,
            ice::Constant_TraitName_RenderDebug,
            ice::Constant_TraitName_DevUI,
            ice::Constant_TraitName_RenderFinish,
        };

        static ice::WorldTemplate const graphics_world_template
        {
            .name = "ice.framework-base.default-graphics-world-template"_sid,
            .traits = graphics_traits,
            .entity_storage = _ecs_storage.get(),
        };

        return graphics_world_template;
    }
    else
    {
        static ice::StringID constexpr graphics_traits[]{
            ice::Constant_TraitName_RenderCamera,
            ice::Constant_TraitName_RenderBase,
            ice::Constant_TraitName_RenderTextureLoader,
            ice::Constant_TraitName_RenderClear,
            ice::Constant_TraitName_RenderSprites,
            ice::Constant_TraitName_RenderTilemap,
            ice::Constant_TraitName_RenderGlyphs,
            ice::Constant_TraitName_RenderPostprocess,
            ice::Constant_TraitName_RenderDebug,
            ice::Constant_TraitName_RenderFinish,
        };

        static ice::WorldTemplate const graphics_world_template
        {
            .name = "ice.framework-base.default-graphics-world-template"_sid,
            .traits = graphics_traits,
            .entity_storage = _ecs_storage.get(),
        };

        return graphics_world_template;
    }
}

void MyGame::on_load_modules(ice::GameServices& sercies) noexcept
{
    ice::ModuleRegister& mod = sercies.module_registry();
    ice::ResourceTracker& res = sercies.resource_system();

    ice::ResourceHandle* const pipelines_module = res.find_resource(u8"urn:iceshard_pipelines.dll"_uri);
    ice::ResourceHandle* const engine_module = res.find_resource(u8"urn:iceshard.dll"_uri);
    ice::ResourceHandle* const vulkan_module = res.find_resource(u8"urn:vulkan_renderer.dll"_uri);
    ice::ResourceHandle* const imgui_module = res.find_resource(u8"urn:imgui_module.dll"_uri);

    ICE_ASSERT(pipelines_module != nullptr, "Missing `iceshard_pipelines.dll` module!");
    ICE_ASSERT(engine_module != nullptr, "Missing `iceshard.dll` module!");
    ICE_ASSERT(vulkan_module != nullptr, "Missing `vulkan_renderer.dll` module!");

    mod.load_module(_allocator, ice::resource_origin(pipelines_module));
    mod.load_module(_allocator, ice::resource_origin(engine_module));
    mod.load_module(_allocator, ice::resource_origin(vulkan_module));

    ice::register_asset_modules(_allocator, mod);

    if (imgui_module != nullptr)
    {
        mod.load_module(_allocator, ice::resource_origin(imgui_module));
    }
}

void MyGame::on_app_startup(ice::Engine& engine) noexcept
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
            [[maybe_unused]]
            auto a1 = archetype_index.register_archetype(ice::ecs::static_validation::Validation_Archetype_1);

            [[maybe_unused]]
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

    _game_gfx_pass->add_stage(ice::Constant_GfxStage_Clear);
    _game_gfx_pass->add_stage(ice::Constant_GfxStage_DrawTilemap, ice::Constant_GfxStage_Clear);
    _game_gfx_pass->add_stage(ice::Constant_GfxStage_DrawSprites, ice::Constant_GfxStage_DrawTilemap, ice::Constant_GfxStage_Clear);
    _game_gfx_pass->add_stage(ice::Constant_GfxStage_Postprocess, ice::Constant_GfxStage_DrawSprites);
    _game_gfx_pass->add_stage(ice::Constant_GfxStage_DrawGlyphs, ice::Constant_GfxStage_Postprocess);
    _game_gfx_pass->add_stage(ice::Constant_GfxStage_DrawDebug, ice::Constant_GfxStage_DrawGlyphs);

    if constexpr (ice::build::is_debug || ice::build::is_develop)
    {
        _game_gfx_pass->add_stage(ice::Constant_GfxStage_DevUI, ice::Constant_GfxStage_Postprocess, ice::Constant_GfxStage_DrawDebug);
        _game_gfx_pass->add_stage(ice::Constant_GfxStage_Finish, ice::Constant_GfxStage_Postprocess, ice::Constant_GfxStage_DevUI);
    }
    else
    {
        _game_gfx_pass->add_stage(ice::Constant_GfxStage_Finish, ice::Constant_GfxStage_Postprocess);
    }

    // Initialize archetypes
    {
        _ecs_archetypes.register_archetype(ice::ecs::Constant_ArchetypeDefinition<ice::Camera, ice::CameraOrtho>);
        _ecs_archetypes.register_archetype(ice::ecs::Constant_ArchetypeDefinition<ice::Camera, ice::CameraPerspective>);
        _ecs_archetypes.register_archetype(ice::ecs::Constant_ArchetypeDefinition<ice::Transform2DStatic, ice::Sprite>);
        _ecs_archetypes.register_archetype(ice::ecs::Constant_ArchetypeDefinition<ice::Transform2DDynamic, ice::Sprite, ice::SpriteTile, ice::Animation, ice::AnimationState, ice::PhysicsBody>);
        _ecs_archetypes.register_archetype(ice::ecs::Constant_ArchetypeDefinition<ice::Transform2DDynamic, ice::Sprite, ice::SpriteTile, ice::Animation, ice::AnimationState, ice::Actor, ice::PhysicsVelocity, ice::PhysicsBody>);

        _ecs_storage = ice::make_unique<ice::ecs::EntityStorage>(_allocator, _allocator, _ecs_archetypes);
    }

    ice::WorldManager& world_manager = engine.world_manager();

    ice::StringID const world_traits[]{
        ice::Constant_TraitName_PhysicsBox2D,
        ice::Constant_TraitName_Tilemap,
        ice::Constant_TraitName_SpriteAnimator,
        ice::Constant_TraitName_Actor,
        ice::Constant_TraitName_GameUI,
    };

    ice::WorldTemplate const world_template{
        .name = "game.test_world"_sid,
        .traits = ice::make_span(world_traits),
        .entity_storage = _ecs_storage.get(),
    };

    _test_world = world_manager.create_world(world_template);
    _test_world->add_trait("game"_sid, this);

    _action_triggers = ice::action::create_trigger_database(_allocator);
    _action_system = ice::action::create_action_system(_allocator, _clock, *_action_triggers);
    ice::action::setup_common_triggers(*_action_triggers);

    using ice::operator""_shard;
    using ice::operator""_shardid;
    using namespace ice::action;

    Action my_action;
    my_action.name = "jump-action"_sid;
    my_action.stage_count = 1;
    my_action.trigger_count = 3;

    ActionStage stages[]{
        ActionStage
        {
            .stage_shardid = "event/player/can_jump_again"_shardid,
            .success_trigger_offset = 0,
            .success_trigger_count = 1,
            .failure_trigger_offset = 1,
            .failure_trigger_count = 0,
            .reset_trigger_offset = 2
        }
    };

    [[maybe_unused]]
    ice::input::InputEvent event_w{ };

    [[maybe_unused]]
    ice::input::InputEvent event_a{ };

    [[maybe_unused]]
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
            .user_shard = "time"_shard | ice::f32{ 0.4f }
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

    ice::WorldManager& world_manager = engine.world_manager();
    world_manager.destroy_world("game.test_world"_sid);

    // Destroy ECS storage
    _ecs_storage = nullptr;
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

    auto const player_arch = ice::ecs::Constant_Archetype<ice::Transform2DDynamic, ice::Sprite, ice::SpriteTile, ice::Animation, ice::AnimationState, ice::Actor, ice::PhysicsVelocity, ice::PhysicsBody>;

    ice::ecs::Entity camera_entity = _current_engine->entity_index().create();
    ice::ecs::Entity player_entity = _current_engine->entity_index().create();

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

    ice::ecs::queue_set_archetype_with_data(
        runner.current_frame().entity_operations(),
        camera_entity,
        ice::ecs::Constant_Archetype<ice::Camera, ice::CameraOrtho>,
        camera,
        orto_values
    );

    ice::ecs::queue_set_archetype_with_data(
        runner.current_frame().entity_operations(),
        player_entity,
        player_arch,
        ice::Animation{ .animation = "cotm_idle"_sid_hash, .speed = 1.f / 60.f },
        ice::Actor{ .type = ice::ActorType::Player },
        ice::Transform2DDynamic{ .position = { 48.f * 2, 448.f, -1.f }, .scale = { 1.f, 0.f } },
        ice::PhysicsBody{ .shape = ice::PhysicsShape::Capsule, .dimensions = { 16.f, 32.f }, .trait_data = nullptr },
        ice::PhysicsVelocity{ .velocity = { 0.1f, 0.f } },
        ice::Sprite{ .material = u8"cotm/cotm_hero" },
        ice::SpriteTile{ .material_tile = { 0, 0 } }
    );

    std::u8string_view const* tilemap_asset = runner.current_frame().create_named_object<std::u8string_view>(
        "tilemap_asset_name"_sid,
        u8"cotm/test_level_2/tiled/0002_Level_1"
    );

    ice::Shard shards[]{
        ice::Shard_WorldActivate | _test_world,
        ice::Shard_LoadTileMap | tilemap_asset,
        ice::Shard_GameUI_Load | u8"ui/test",
        ice::Shard_GameUI_Load | u8"ui/test2",
        ice::Shard_GameUI_Load | u8"ui/test3",
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
    using ice::operator""_sid_hash;

    runner.graphics_frame().enqueue_pass("default"_sid, "game.render"_sid, _game_gfx_pass.get());

    static ice::DrawTextCommand const draw_text{
        .position = { 90, 90 },
        .text = u8"Hello, World!",
        .font = u8"calibri",
        .font_size = 20,
    };

    static ice::DrawTextCommand const draw_text2{
        .position = { 90, 140 },
        .text = u8"わたし Daniel!",
        .font = u8"yumin",
        .font_size = 14,
    };

    // Just to make testing more nice, skip this command on the first frame, so it doesn't block loading of the tilemap.
    //  TODO: Make the tilemap loaded async, so it will not wait for other unrelated assets to be loaded.
    static int i = 0;
    if (i != 0)
    {
        ice::shards::push_back(frame.shards(), ice::Shard_DrawTextCommand | &draw_text);
        ice::shards::push_back(frame.shards(), ice::Shard_DrawTextCommand | &draw_text2);
    }

    i = 1;

    //ice::shards::push_back(frame.shards(), ice::Shard_DrawTextCommand | &draw_text2);

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

    ice::Shard const player_entity_created = ice::shards::find_last_of(frame.shards(), ice::ecs::Shard_EntityCreated);

    if (player_entity_created != ice::Shard_Invalid)
    {
        //ice::ecs::queue_remove_entity(
        //    frame.entity_operations(),
        //    ice::shard_shatter<ice::ecs::EntityHandle>(player_entity_created)
        //);

        ice::shards::push_back(
            frame.shards(),
            ice::Shard_SetDefaultCamera | "camera.default"_sid_hash
        );
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
