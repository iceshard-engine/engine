/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/framework_app.hxx>
#include <ice/framework_module.hxx>


#include <ice/engine.hxx>
#include <ice/engine_frame.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_shards.hxx>
#include <ice/engine_devui.hxx>
#include <ice/devui_context.hxx>
#include <ice/world/world.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/world/world_updater.hxx>
#include <ice/world/world_trait_module.hxx>
#include <ice/ecs/ecs_archetype_index.hxx>
#include <ice/ecs/ecs_entity_storage.hxx>
#include <ice/ecs/ecs_entity_operations.hxx>

#include <ice/gfx/gfx_stage.hxx>
#include <ice/gfx/gfx_runner.hxx>
#include <ice/gfx/gfx_context.hxx>
#include <ice/gfx/gfx_graph.hxx>
#include <ice/gfx/gfx_graph_runtime.hxx>
#include <ice/gfx/gfx_shards.hxx>

#include <ice/render/render_image.hxx>
#include <ice/render/render_swapchain.hxx>

#include <ice/resource_tracker.hxx>
#include <ice/module_register.hxx>
#include <ice/asset_types.hxx>
#include <ice/profiler.hxx>
#include <ice/uri.hxx>
#include <ice/log.hxx>
#include <thread>

using namespace ice;

static constexpr LogTagDefinition LogGame = ice::create_log_tag(LogTag::Game, "TestGame");

struct TestTrait : public ice::Trait
{
    ice::Timer timer;

    TestTrait(ice::TraitContext& context) noexcept
        : ice::Trait{ context }
    {
        _context.bind<&TestTrait::logic>();
        _context.bind<&TestTrait::gfx>(ice::gfx::ShardID_GfxFrameUpdate);
    }

    auto activate(ice::WorldStateParams const& update) noexcept -> ice::Task<> override
    {
        timer = ice::timer::create_timer(update.clock, 1_Ts);
        co_return;
    }

    auto logic(ice::EngineFrameUpdate const& update) noexcept -> ice::Task<>
    {
        if (ice::timer::update(timer))
        {
            ICE_LOG(LogSeverity::Info, LogTag::Game, "TestTrait::logic");
        }

        co_await update.thread.tasks;
        IPT_ZONE_SCOPED;
        co_return;
    }

    auto gfx(ice::gfx::GfxFrameUpdate const& update) noexcept -> ice::Task<>
    {
        IPT_ZONE_SCOPED;
        co_return;
    }
};

namespace icetm = ice::detail::world_traits;

auto test_factory(ice::Allocator& alloc, ice::TraitContext& context, void*) noexcept -> UniquePtr<ice::Trait>
{
    return ice::make_unique<TestTrait>(alloc, context);
}

bool test_reg_traits(ice::TraitArchive& arch) noexcept
{
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

    auto rendergraph(ice::gfx::GfxContext& ctx) noexcept -> ice::UniquePtr<ice::gfx::GfxGraphRuntime> override;

private:
    ice::Allocator& _allocator;

    ice::UniquePtr<ice::gfx::GfxGraph> _graph;
    ice::UniquePtr<ice::gfx::GfxGraphRuntime> _graph_runtime;

    ice::UniquePtr<ice::ecs::ArchetypeIndex> _archetype_index;
    ice::UniquePtr<ice::ecs::EntityStorage> _entity_storage;

    bool _first_time;
};

TestGame::TestGame(ice::Allocator& alloc) noexcept
    : _allocator{ alloc }
    , _first_time{ true }
{
}

void TestGame::on_setup(ice::framework::State const& state) noexcept
{
    ice::ModuleRegister& mod = state.modules;
    ice::ResourceTracker& res = state.resources;

    ice::HeapString<> pipelines_module = ice::resolve_dynlib_path(res, _allocator, "iceshard_pipelines_mobile");
    ice::HeapString<> vulkan_module = ice::resolve_dynlib_path(res, _allocator, "vulkan_renderer");

    mod.load_module(_allocator, pipelines_module);
    mod.load_module(_allocator, vulkan_module);

    _archetype_index = ice::make_unique<ice::ecs::ArchetypeIndex>(_allocator, _allocator);
    _entity_storage = ice::make_unique<ice::ecs::EntityStorage>(_allocator, _allocator, *_archetype_index);
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
            "test"_sid,
            "test2"_sid,
            ice::TraitID_GfxShaderStorage,
            ice::devui_trait_name()
        };

        engine.worlds().create_world(
            { .name = "world"_sid, .traits = traits, .entity_storage = *_entity_storage }
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

auto TestGame::rendergraph(ice::gfx::GfxContext& ctx) noexcept -> ice::UniquePtr<ice::gfx::GfxGraphRuntime>
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

    return create_graph_runtime(_allocator, ctx, *_graph);
}

auto ice::framework::create_game(ice::Allocator& alloc) noexcept -> ice::UniquePtr<Game>
{
    return ice::make_unique<TestGame>(alloc, alloc);
}
