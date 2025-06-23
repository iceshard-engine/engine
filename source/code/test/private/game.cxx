/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
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
#include <ice/devui_imgui.hxx>

#include <ice/gfx/gfx_stage.hxx>
#include <ice/gfx/gfx_runner.hxx>
#include <ice/gfx/gfx_context.hxx>
#include <ice/gfx/gfx_graph.hxx>
#include <ice/gfx/gfx_graph_runtime.hxx>
#include <ice/gfx/gfx_shards.hxx>

#include <ice/render/render_image.hxx>
#include <ice/render/render_swapchain.hxx>
#include <ice/ecs/ecs_entity_operations.hxx>

#include <ice/mem_allocator_snake.hxx>
#include <ice/task_debug_allocator.hxx>
#include <ice/task_utils.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/module_register.hxx>
#include <ice/asset_types.hxx>
#include <ice/profiler.hxx>
#include <ice/uri.hxx>
#include <ice/log.hxx>
#include <thread>

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

    WorldActivationTrait(ice::TraitContext& context) noexcept
        : ice::Trait{ context }
        , ice::DevUIWidget{ { .category = "Test", .name = "Test" } }
    {
        ice::devui_register_widget(this);
        _context.bind<&WorldActivationTrait::logic>();
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
};

struct C1
{
    static constexpr ice::StringID Identifier = "iceshard.test.ecs.C1"_sid;
    int x;
};

struct C2
{
    static constexpr ice::StringID Identifier = "iceshard.test.ecs.C2"_sid;
    float y;
};

struct TestTrait : public ice::Trait
{
    TestTrait(ice::Allocator& alloc, ice::TraitContext& context) noexcept
        : ice::Trait{ context }
        , _alloc{ alloc }
    {
        _context.bind<&TestTrait::logic>();
        _context.bind<&TestTrait::gfx>(ice::gfx::ShardID_GfxFrameUpdate);
    }

    ice::Allocator& _alloc;
    ice::Timer timer;

    ice::ecs::EntityOperations* _ops;
    ice::ecs::Entity _my_entity[10000];

    auto activate(ice::WorldStateParams const& update) noexcept -> ice::Task<> override
    {

        update.engine.entity_index().create_many(_my_entity);
        _ops = ice::addressof(update.world.entity_operations());
        _ops->set("test-arch", _my_entity);

        ICE_LOG(LogSeverity::Retail, LogTag::Game, "Test Activated!");
        timer = ice::timer::create_timer(update.clock, 100_Tms);
        co_return;
    }

    auto deactivate(ice::WorldStateParams const& update) noexcept -> ice::Task<> override
    {
        update.engine.entity_index().destroy_many(_my_entity);

        query<ice::ecs::Entity>().tags<C1, C2>().for_each_block(
            [&](ice::ucount count, ice::ecs::Entity const* entities) noexcept
            {
                _ops->destroy({ entities, count });
            }
        );

        ICE_LOG(LogSeverity::Retail, LogTag::Game, "Test Deactivated!");
        co_return;
    }

    auto logic(ice::EngineFrameUpdate const& update) noexcept -> ice::Task<>
    {
        ice::ecs::Query q = query<C1&, C2&>();

        ice::Array<ice::Task<>> tasks{ update.frame.allocator() };
        ice::array::reserve(tasks, q.block_count());

        q.for_each_block([&](ice::ucount count, C1* c1p, C2* c2p) noexcept
        {
            ice::array::push_back(tasks, [](ice::ucount count, C1* c1p, C2* c2p) noexcept -> ice::Task<>
            {
                IPT_ZONE_SCOPED_NAMED("block for-each");
                for (ice::u32 idx = 0; idx < count; ++idx)
                {
                    IPT_ZONE_SCOPED;
                    c1p[idx].x += 1;
                    c2p[idx].y += 1.0f / ((ice::f32) c1p[idx].x);
                }
                co_return;
            }(count, c1p, c2p));
        });

        q.for_each_entity(
            [](C1& c1, C2& c2) noexcept
            {
                IPT_ZONE_SCOPED;
                c1.x += 1;
                c2.y += 1.0f / ((ice::f32) c1.x);
            }
        );

        if (ice::timer::update(timer))
        {
            ICE_LOG(LogSeverity::Info, LogTag::Game, "TestTrait::logic");
        }

        ICE_LOG(LogSeverity::Debug, LogTag::Game, "{}", std::hash<std::thread::id>{}(std::this_thread::get_id()));
        co_await ice::await_scheduled_on(tasks, update.thread.tasks, update.thread.main);
        ICE_LOG(LogSeverity::Debug, LogTag::Game, "{}", std::hash<std::thread::id>{}(std::this_thread::get_id()));
        co_return;
    }

    auto gfx(ice::gfx::GfxFrameUpdate const& update) noexcept -> ice::Task<>
    {
        IPT_ZONE_SCOPED;
        co_return;
    }
};

namespace icetm = ice::detail::world_traits;

auto act_factory(ice::Allocator& alloc, ice::TraitContext& context, void*) noexcept -> UniquePtr<ice::Trait>
{
    return ice::make_unique<WorldActivationTrait>(alloc, context);
}
auto test_factory(ice::Allocator& alloc, ice::TraitContext& context, void*) noexcept -> UniquePtr<ice::Trait>
{
    return ice::make_unique<TestTrait>(alloc, alloc, context);
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

    ice::HeapString<> shader_tools = ice::resolve_dynlib_path(res, _allocator, "shader_tools");
    ice::HeapString<> pipelines_module = ice::resolve_dynlib_path(res, _allocator, "iceshard_pipelines");
    ice::HeapString<> vulkan_module = ice::resolve_dynlib_path(res, _allocator, "vulkan_renderer");

    mod.load_module(_allocator, shader_tools);
    mod.load_module(_allocator, pipelines_module);
    mod.load_module(_allocator, vulkan_module);

    state.archetypes.new_archetype<C1, C2>("test-arch");
}

void TestGame::on_shutdown(ice::framework::State const& state) noexcept
{
    ICE_LOG(LogSeverity::Info, LogGame, "Goodbye, World!");
}

void TestGame::on_resume(ice::Engine& engine) noexcept
{
    // Create and forget about the null entity.
    engine.entity_index().create();

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
        // GfxResource const c0 = _graph->get_resource("color"_sid, GfxResourceType::RenderTarget);
        GfxResource const fb = _graph->get_framebuffer();

        GfxGraphStage const stages1[]{
            {.name = "clear"_sid, .outputs = { &fb, 1 }},
        };
        GfxGraphStage const stages2[]{
            {.name = "iceshard.devui"_sid, .outputs = {&fb, 1}}
        };
        GfxGraphPass const pass1{ .name = "test1"_sid, .stages = stages1 };
        GfxGraphPass const pass2{ .name = "test2"_sid, .stages = stages2 };
        _graph->add_pass(pass1);
        _graph->add_pass(pass2);
    }

    return create_graph_runtime(_allocator, device, *_graph);
}
