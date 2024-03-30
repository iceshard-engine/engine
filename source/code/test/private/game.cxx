/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "game.hxx"

#include <ice/framework_module.hxx>

#include <ice/engine.hxx>
#include <ice/engine_frame.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_shards.hxx>
#include <ice/engine_devui.hxx>
#include <ice/world/world.hxx>
#include <ice/world/world_updater.hxx>
#include <ice/world/world_trait_module.hxx>
#include <ice/devui_widget.hxx>
#include <ice/devui_context.hxx>

#include <ice/gfx/gfx_stage.hxx>
#include <ice/gfx/gfx_runner.hxx>
#include <ice/gfx/gfx_context.hxx>
#include <ice/gfx/gfx_graph.hxx>
#include <ice/gfx/gfx_graph_runtime.hxx>
#include <ice/gfx/gfx_shards.hxx>

#include <ice/render/render_image.hxx>
#include <ice/render/render_swapchain.hxx>

#include <ice/task_debug_allocator.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/module_register.hxx>
#include <ice/asset_types.hxx>
#include <ice/profiler.hxx>
#include <ice/uri.hxx>
#include <ice/log.hxx>
#include <thread>

#include <imgui/imgui.h>
#undef assert

static constexpr LogTagDefinition LogGame = ice::create_log_tag(LogTag::Game, "TestGame");

struct GameTasksDebugAllocator final : public ice::Module<GameTasksDebugAllocator>
{
    static void set_allocator(ice::ProxyAllocator* allocator) noexcept
    {
        _allocator_ptr = allocator;
    }

    static void v1_api(ice::detail::DebugAllocatorAPI& api) noexcept
    {
        ICE_ASSERT_CORE(_allocator_ptr != nullptr);
        api.allocator_ptr = _allocator_ptr;
        api.allocator_pool = "tasks";
    }

    static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiator auto const& negotiator) noexcept
    {
        negotiator.register_api(v1_api);
        return true;
    }

    IS_WORKAROUND_MODULE_INITIALIZATION(GameTasksDebugAllocator);

private:
    static ice::ProxyAllocator* _allocator_ptr;
};

ice::ProxyAllocator* GameTasksDebugAllocator::_allocator_ptr = nullptr;

auto ice::framework::create_game(ice::Allocator& alloc) noexcept -> ice::UniquePtr<Game>
{
    return ice::make_unique<TestGame>(alloc, alloc);
}

struct WorldActivationTrait : ice::Trait, ice::DevUIWidget
{
    bool is_active = false;
    bool do_active = false;

    WorldActivationTrait() noexcept
        : ice::DevUIWidget{ { .category = "Test", .name = "Test" } }
    {
        ice::devui_register_widget(this);
    }

    void build_content() noexcept override
    {
        ImGui::Checkbox("World Active", &do_active);
    }

    auto logic(ice::EngineFrameUpdate const& update) noexcept -> ice::Task<>
    {
        if (is_active ^ do_active)
        {
            if (do_active)
            {
                shards::push_back(update.frame.shards(), ShardID_WorldActivate | "world2"_sid_hash);
            }
            else
            {
                shards::push_back(update.frame.shards(), ShardID_WorldDeactivate | "world2"_sid_hash);
            }
            is_active = do_active;
        }
        co_return;
    }

    void gather_tasks(ice::TraitTaskRegistry& task_launcher) noexcept
    {
        task_launcher.bind<&WorldActivationTrait::logic>();
    }
};

struct TestTrait : public ice::Trait
{
    ice::Timer timer;

    auto activate(ice::WorldStateParams const& update) noexcept -> ice::Task<> override
    {
        ICE_LOG(LogSeverity::Retail, LogTag::Game, "Test Activated!");
        timer = ice::timer::create_timer(update.clock, 0.1f);
        co_return;
    }

    auto deactivate(ice::WorldStateParams const& update) noexcept -> ice::Task<> override
    {
        ICE_LOG(LogSeverity::Retail, LogTag::Game, "Test Deactivated!");
        co_return;
    }

    void gather_tasks(ice::TraitTaskRegistry& task_launcher) noexcept
    {
        task_launcher.bind<&TestTrait::logic>();
        task_launcher.bind<&TestTrait::gfx>(ice::gfx::ShardID_GfxFrameUpdate);
    }

    auto logic(ice::EngineFrameUpdate const& update) noexcept -> ice::Task<>
    {
        if (ice::timer::update(timer))
        {
            ICE_LOG(LogSeverity::Info, LogTag::Game, "TestTrait::logic");
        }

        co_return;
    }

    auto gfx(ice::gfx::GfxFrameUpdate const& update) noexcept -> ice::Task<>
    {
        IPT_ZONE_SCOPED;
        co_return;
    }
};

namespace icetm = ice::detail::world_traits;

auto act_factory(ice::Allocator& alloc, void*) noexcept -> UniquePtr<ice::Trait>
{
    return ice::make_unique<WorldActivationTrait>(alloc);
}
auto test_factory(ice::Allocator& alloc, void*) noexcept -> UniquePtr<ice::Trait>
{
    return ice::make_unique<TestTrait>(alloc);
}

bool test_reg_traits(ice::TraitArchive& arch) noexcept
{
    arch.register_trait({ .name = "act"_sid, .fn_factory = act_factory });
    arch.register_trait({ .name = "test"_sid, .fn_factory = test_factory });
    return true;
}

struct TestModule : ice::Module<TestModule>
{
    static void v1_traits_api(ice::detail::world_traits::TraitsModuleAPI& api) noexcept
    {
        api.register_traits_fn = test_reg_traits;
    }

    static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiator auto const& negotiator) noexcept
    {
        return negotiator.register_api(v1_traits_api);
    }

    IS_WORKAROUND_MODULE_INITIALIZATION(TestModule);
};

TestGame::TestGame(ice::Allocator& alloc) noexcept
    : _allocator{ alloc }
    , _tasks_alloc{ _allocator, "tasks" }
    , _first_time{ true }
{
    GameTasksDebugAllocator::set_allocator(&_tasks_alloc);
}

void TestGame::on_setup(ice::framework::State const& state) noexcept
{
    // ICE_LOG(LogSeverity::Info, LogGame, "Hello, World!");

    ice::ModuleRegister& mod = state.modules;
    ice::ResourceTracker& res = state.resources;

    ice::HeapString<> pipelines_module = ice::resolve_dynlib_path(res, _allocator, "iceshard_pipelines");
    ice::HeapString<> vulkan_module = ice::resolve_dynlib_path(res, _allocator, "vulkan_renderer");

    mod.load_module(_allocator, pipelines_module);
    mod.load_module(_allocator, vulkan_module);
}

void TestGame::on_shutdown(ice::framework::State const& state) noexcept
{
    ICE_LOG(LogSeverity::Info, LogGame, "Goodbye, World!");
}

void TestGame::on_resume(ice::Engine& engine) noexcept
{
    if (_first_time)
    {
        _first_time = false;

        using ice::operator""_sid;

        ice::StringID traits[]{
            "act"_sid,
            "test2"_sid,
            ice::devui_trait_name(),
            ice::TraitID_GfxShaderStorage
        };
        ice::StringID traits2[]{
            "test"_sid,
        };

        engine.worlds().create_world(
            { .name = "world"_sid, .traits = traits }
        );
        engine.worlds().create_world(
            { .name = "world2"_sid, .traits = traits2 }
        );
    }
}

void TestGame::on_update(ice::Engine& engine, ice::EngineFrame& frame) noexcept
{
    using namespace ice;

    shards::push_back(frame.shards(), ShardID_WorldActivate | "world"_sid_hash);
}

void TestGame::on_suspend(ice::Engine& engine) noexcept
{
}

auto TestGame::rendergraph(ice::gfx::GfxContext& device) noexcept -> ice::UniquePtr<ice::gfx::GfxGraphRuntime>
{
    using ice::operator""_sid;
    using namespace ice::gfx;

    _graph = create_graph(_allocator);
    {
        GfxResource const c0 = _graph->get_resource("color"_sid, GfxResourceType::RenderTarget);
        GfxResource const fb = _graph->get_framebuffer();

        GfxGraphStage const stages1[]{
            {.name = "clear"_sid, .outputs = { &c0, 1 }},
        };
        GfxGraphStage const stages2[]{
            {.name = "copy"_sid, .inputs = { &c0, 1 }, .outputs = { &fb, 1 }}
        };
        GfxGraphPass const pass1{ .name = "test1"_sid, .stages = stages1 };
        GfxGraphPass const pass2{ .name = "test2"_sid, .stages = stages2 };
        _graph->add_pass(pass1);
        _graph->add_pass(pass2);
    }

    return create_graph_runtime(_allocator, device, *_graph);
}

#if 0
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
#include <ice/gfx/gfx_context.hxx>
#include <ice/gfx/gfx_frame.hxx>
#include <ice/gfx/gfx_pass.hxx>
#include <ice/gfx/gfx_runner.hxx>
#include <ice/render/render_image.hxx>
#include <ice/render/render_swapchain.hxx>

#include <ice/input/input_event.hxx>
#include <ice/input/input_keyboard.hxx>
#include <ice/input/input_tracker.hxx>

#include <ice/task_utils.hxx>
#include <ice/module_register.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/resource_provider.hxx>
#include <ice/asset_storage.hxx>
#include <ice/resource.hxx>
#include <ice/assert.hxx>
#include <ice/shard.hxx>
#include <ice/shard_payloads.hxx>

#include <ice/ui_asset.hxx>
#include <ice/ui_resource.hxx>

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
    , _ecs_storage{ }
    , _game_gfx_pass{ }
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
            ice::Constant_TraitName_RenderUI,
            ice::Constant_TraitName_RenderGlyphs,
            ice::Constant_TraitName_RenderPostprocess,
            ice::Constant_TraitName_RenderDebug,
            ice::Constant_TraitName_DevUI,
            ice::Constant_TraitName_RenderFinish,
        };

        static ice::WorldTemplate graphics_world_template
        {
            .name = "ice.framework-base.default-graphics-world-template"_sid,
                .traits = graphics_traits,
                .entity_storage = _ecs_storage.get(),
        };

        graphics_world_template.entity_storage = _ecs_storage.get();
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
            ice::Constant_TraitName_RenderUI,
            ice::Constant_TraitName_RenderGlyphs,
            ice::Constant_TraitName_RenderPostprocess,
            ice::Constant_TraitName_RenderDebug,
            ice::Constant_TraitName_RenderFinish,
        };

        static ice::WorldTemplate graphics_world_template
        {
            .name = "ice.framework-base.default-graphics-world-template"_sid,
                .traits = graphics_traits,
                .entity_storage = _ecs_storage.get(),
        };

        graphics_world_template.entity_storage = _ecs_storage.get();
        return graphics_world_template;
    }
}

void MyGame::on_load_modules(ice::GameServices& services) noexcept
{
    ice::ModuleRegister& mod = services.module_registry();
    ice::ResourceTracker& res = services.resource_system();

    // Maybe we should move the location where we can do replacements?
    //{
    //    [[maybe_unused]]
    //    ice::ResourceHandle* tsa = res.find_resource("file:/data/cotm/tileset_a.png"_uri);
    //    ice::wait_for(res.set_resource("urn:cotm/tileset_ab.png"_uri, tsa));
    //}

    ice::ResourceHandle* const pipelines_module = res.find_resource("urn:iceshard_pipelines.dll"_uri);
    ice::ResourceHandle* const engine_module = res.find_resource("urn:iceshard.dll"_uri);
    ice::ResourceHandle* const vulkan_module = res.find_resource("urn:vulkan_renderer.dll"_uri);
    ice::ResourceHandle* const imgui_module = res.find_resource("urn:imgui_module.dll"_uri);

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

                ice::ecs::queue_remove_entity(entity_operations, ice::shard_shatter<ice::ecs::EntityHandle>(shards._data[3], ice::ecs::EntityHandle::Invalid));
                ice::ecs::queue_set_archetype(entity_operations, entities[3], a1);
                entity_storage.execute_operations(entity_operations, shards);
            }
        }
        entity_index.destroy_many(entities);
    }

    _game_gfx_pass = ice::gfx::create_dynamic_pass(_allocator);
    _game_gfx_pass->add_stage(ice::Constant_GfxStage_Clear);
    _game_gfx_pass->add_stage(ice::Constant_GfxStage_DrawTilemap, ice::Constant_GfxStage_Clear);
    _game_gfx_pass->add_stage(ice::Constant_GfxStage_DrawSprites, ice::Constant_GfxStage_DrawTilemap, ice::Constant_GfxStage_Clear);
    _game_gfx_pass->add_stage(ice::Constant_GfxStage_Postprocess, ice::Constant_GfxStage_DrawSprites);
    _game_gfx_pass->add_stage(ice::Constant_GfxStage_DrawUI, ice::Constant_GfxStage_Postprocess);
    _game_gfx_pass->add_stage(ice::Constant_GfxStage_DrawGlyphs, ice::Constant_GfxStage_DrawUI);
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
        engine.world_trait_archive().register_archetypes(_ecs_archetypes);

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
            .traits = ice::Span{ world_traits },
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
        .left_right = { 0.f, (ice::f32)extent.x / 2.f },
            .bottom_top = { 0.f, (ice::f32)extent.y / 2.f },
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
        ice::Animation{.animation = "cotm_idle"_sid_hash, .speed = 1.f / 60.f },
        ice::Actor{.type = ice::ActorType::Player },
        ice::Transform2DDynamic{.position = { 48.f * 2, 448.f, -1.f }, .scale = { 1.f, 0.f } },
        ice::PhysicsBody{.shape = ice::PhysicsShape::Capsule, .dimensions = { 16.f, 32.f }, .trait_data = nullptr },
        ice::PhysicsVelocity{.velocity = { 0.1f, 0.f } },
        ice::Sprite{.material = "local/cotm_hero" },
        ice::SpriteTile{.material_tile = { 0, 0 } }
    );

    ice::String const* tilemap_asset = runner.current_frame().storage().create_named_object<ice::String>(
        "tilemap_asset_name"_sid,
        "local/maps/level_0"
    );

    ice::Shard shards[]{
        ice::Shard_WorldActivate | _test_world,
        ice::Shard_LoadTileMap | tilemap_asset,
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

    static constexpr ice::String temp_ui_ids[]{
        "text_start",
        "text_settings",
        "text_credits",
        "text_exit",
    };
    static constexpr ice::String temp_ui_strings[]{
        "Start",
        "Settings",
        "Credits",
        "Exit",
    };

    ice::shards::inspect_each<char const*>(
        runner.previous_frame().shards(),
        ice::Shard_GameUI_Loaded,
        [&frame, this](char const* page_name) noexcept
        {
            for (ice::u32 idx = 0; idx < ice::count(temp_ui_strings); ++idx)
            {
                ice::String* str = frame.storage().create_named_object<ice::String>(ice::stringid(temp_ui_strings[idx]), temp_ui_strings[idx]);
                ice::UpdateUIResource* ures = frame.storage().create_named_object<ice::UpdateUIResource>(ice::stringid(temp_ui_ids[idx]));
                ures->page = page_name;
                ures->resource_data = ice::to_const(str);
                ures->resource = ice::stringid(temp_ui_ids[idx]);
                ures->resource_type = ice::ui::ResourceType::String;
                ice::shards::push_back(frame.shards(), ice::Shard_GameUI_UpdateResource | ice::to_const(ures));
            }
            _menu = page_name;
        }
    );

    if (frame.index() == 2)
    {
        ice::shards::push_back(frame.shards(),
            ice::Shard_GameUI_Load | "ui/test"
        );
    }

    bool was_active = _active;
    for (ice::input::InputEvent const& event : frame.input_events())
    {
        if (ice::input::input_identifier(ice::input::DeviceType::Keyboard, ice::input::KeyboardKey::Escape) == event.identifier)
        {
            if (_menu != nullptr && (event.value.button.state.clicked || event.value.button.state.repeat > 0))
            {
                if (_menu_visible)
                {
                    ice::shards::push_back(frame.shards(), ice::Shard_GameUI_Hide | _menu);
                }
                else
                {
                    ice::shards::push_back(frame.shards(), ice::Shard_GameUI_Show | _menu);
                }
                _menu_visible = !_menu_visible;
            }
        }
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

    ice::ecs::EntityHandle eh;
    ice::Shard const player_entity_created = ice::shards::find_first_of(runner.previous_frame().shards(), ice::ecs::Shard_EntityCreated);

    if (ice::shard_inspect(player_entity_created, eh))
    {
        //ICE_ASSERT(ice::ecs::entity_handle_info(eh).entity == ice::ecs::Entity{}, "{}", eh);
        ICE_LOG(ice::LogSeverity::Debug, ice::LogTag::Game, "{}", eh);
    }

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
#endif
