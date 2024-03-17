/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/framework_app.hxx>
#include <ice/framework_module.hxx>

#include <ice/engine.hxx>
#include <ice/engine_frame.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_shards.hxx>
#include <ice/engine_devui.hxx>
#include <ice/world/world.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/world/world_updater.hxx>
#include <ice/world/world_trait_module.hxx>

#include <ice/gfx/gfx_stage.hxx>
#include <ice/gfx/gfx_runner.hxx>
#include <ice/gfx/gfx_context.hxx>
#include <ice/gfx/gfx_graph.hxx>
#include <ice/gfx/gfx_graph_runtime.hxx>
#include <ice/gfx/gfx_shards.hxx>
#include <ice/gfx/gfx_stage_registry.hxx>

#include <ice/render/render_image.hxx>
#include <ice/render/render_swapchain.hxx>
#include <ice/render/render_pipeline.hxx>
#include <ice/render/render_device.hxx>
#include <ice/render/render_command_buffer.hxx>
#include <ice/render/render_buffer.hxx>

#include <ice/input/input_event.hxx>
#include <ice/input/input_mouse.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/module_register.hxx>
#include <ice/asset_types.hxx>
#include <ice/profiler.hxx>
#include <ice/uri.hxx>
#include <ice/log.hxx>
#include <thread>

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

    auto rendergraph(ice::gfx::GfxContext& context) noexcept -> ice::UniquePtr<ice::gfx::GfxGraphRuntime> override;

private:
    ice::Allocator& _allocator;

    ice::UniquePtr<ice::gfx::GfxGraph> _graph;
    ice::UniquePtr<ice::gfx::GfxGraphRuntime> _graph_runtime;

    bool _first_time;
};

static constexpr LogTagDefinition LogGame = ice::create_log_tag(LogTag::Game, "TestGame");

auto ice::framework::create_game(ice::Allocator& alloc) noexcept -> ice::UniquePtr<Game>
{
    return ice::make_unique<TestGame>(alloc, alloc);
}

#if 0
struct TestTrait : public ice::Trait, public ice::gfx::GfxStage
{
    ice::Timer timer;

    auto activate(ice::EngineWorldUpdate const& update) noexcept -> ice::Task<> override
    {
        timer = ice::timer::create_timer(update.clock, 1.0f);

        ice::Asset s0 = update.assets.bind(ice::render::AssetType_Shader, "shaders/color/blue-vert");
        ice::Asset s1 = update.assets.bind(ice::render::AssetType_Shader, "shaders/color/blue-frag");
        sd[0] = co_await s0[AssetState::Baked];
        sd[1] = co_await s1[AssetState::Baked];
        co_return;
    }

    void gather_tasks(ice::TraitTaskLauncher& task_launcher) noexcept override
    {
        task_launcher.bind<&TestTrait::logic>();
        task_launcher.bind<&TestTrait::gfx_s>(ice::gfx::ShardID_GfxStartup);
        task_launcher.bind<&TestTrait::gfx>(ice::gfx::ShardID_GfxFrameUpdate);
        task_launcher.bind<&TestTrait::gfx_e>(ice::gfx::ShardID_GfxShutdown);
    }

    auto logic(ice::EngineFrameUpdate const& update) noexcept -> ice::Task<>
    {
        if (ice::timer::update(timer))
        {
            ICE_LOG(LogSeverity::Info, LogTag::Game, "TestTrait::logic");
        }

        using input::MouseInput;

        ice::vec2u pos{};
        ice::shards::inspect_each<ice::input::InputEvent>(
            update.frame.shards(),
            ice::ShardID_InputEvent,
            [&](ice::input::InputEvent input) noexcept
            {
                if (ice::input::input_identifier_device(input.identifier) == ice::input::DeviceType::Mouse)
                {
                    MouseInput const input_source = MouseInput(ice::input::input_identifier_value(input.identifier));

                    switch (input_source)
                    {
                    case MouseInput::PositionX:
                        pos.x = (ice::f32)input.value.axis.value_i32;
                        break;
                    case MouseInput::PositionY:
                        pos.y = (ice::f32)input.value.axis.value_i32;
                        break;
                    }
                }
            }
        );
        co_return;
    }

    auto gfx_s(ice::gfx::GfxStateChange const& params) noexcept -> ice::Task<>
    {
        IPT_ZONE_SCOPED;
        using namespace ice::render;
        RenderDevice& d = params.device.device();

        s[0] = d.create_shader({ .shader_data = sd[0] });
        s[1] = d.create_shader({ .shader_data = sd[1] });

        PipelineLayoutInfo pli{ };
        pl = d.create_pipeline_layout(pli);
        PipelineInfo pi{};
        pi.cull_mode = CullMode::Disabled;
        pi.depth_test = false;
        pi.front_face = FrontFace::ClockWise;
        pi.primitive_topology = PrimitiveTopology::TriangleList;
        pi.renderpass = params.renderpass;
        pi.layout = pl;
        pi.shaders = s;
        ShaderStageFlags ss[]{ ShaderStageFlags::VertexStage, ShaderStageFlags::FragmentStage };
        pi.shaders_stages = ss;
        pi.subpass_index = 1;

        ShaderInputAttribute attrib[]{
            ShaderInputAttribute{ .location = 0, .offset = 0, .type = ShaderAttribType::Vec2f }
        };
        ShaderInputBinding bindings[]{
            ShaderInputBinding{ .binding = 0, .stride = 8, .instanced = false, .attributes = attrib }
        };
        pi.shader_bindings = bindings;
        p = d.create_pipeline(pi);
        vb = d.create_buffer(BufferType::Vertex, sizeof(ice::vec2f) * 3);

        co_return;
    }

    auto gfx(ice::gfx::GfxFrameUpdate const& update) noexcept -> ice::Task<>
    {
        ice::vec2f triangle[]{
            { -0.5, 0.5, },
            { 0.5, 0.5, },
            { 0.5, -0.5, },
        };

        using namespace ice::render;
        RenderDevice& d = update.context.device();
        BufferUpdateInfo bui[]{
            BufferUpdateInfo{.buffer = vb, .data = ice::data_view(triangle)}
        };
        d.update_buffers(bui);

        update.stages.add_stage("copy"_sid, this);
        IPT_ZONE_SCOPED;
        co_return;
    }

    void draw(
        ice::EngineFrame const& frame,
        ice::render::CommandBuffer cmds,
        ice::render::RenderCommands& api
    ) const noexcept override
    {
        api.bind_vertex_buffer(cmds, vb, 0);
        api.bind_pipeline(cmds, p);
        api.draw(cmds, 3, 1, 0, 0);
    }

    auto gfx_e(ice::gfx::GfxStateChange const& params) noexcept -> ice::Task<>
    {
        using namespace ice::render;
        RenderDevice& d = params.device.device();
        d.destroy_pipeline_layout(pl);
        d.destroy_pipeline(p);
        d.destroy_shader(s[0]);
        d.destroy_shader(s[1]);
        d.destroy_buffer(vb);

        IPT_ZONE_SCOPED;
        co_return;
    }

    ice::Data sd[2];
    ice::render::Pipeline p;
    ice::render::PipelineLayout pl;
    ice::render::Shader s[2];
    ice::render::Buffer vb;
};

namespace icetm = ice::detail::world_traits;

auto test_factory(ice::Allocator& alloc, void*) noexcept -> UniquePtr<ice::Trait>
{
    return ice::make_unique<TestTrait>(alloc);
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

    static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiator const& negotiator) noexcept
    {
        return negotiator.register_api(v1_traits_api);
    }

    IS_WORKAROUND_MODULE_INITIALIZATION(TestModule);
};
#endif

TestGame::TestGame(ice::Allocator& alloc) noexcept
    : _allocator{ alloc }
    , _first_time{ true }
{
	//TestModule::module_info();
}

void TestGame::on_setup(ice::framework::State const& state) noexcept
{
    // ICE_LOG(LogSeverity::Info, LogGame, "Hello, World!");

    ice::ModuleRegister& mod = state.modules;
    ice::ResourceTracker& res = state.resources;

    ice::HeapString<> pipelines_module = ice::resolve_dynlib_path(res, _allocator, "iceshard_pipelines_mobile");
    ice::HeapString<> vulkan_module = ice::resolve_dynlib_path(res, _allocator, "vulkan_renderer");
    ice::HeapString<> imgui_module = ice::resolve_dynlib_path(res, _allocator, "imgui_module");

    mod.load_module(_allocator, pipelines_module);
    mod.load_module(_allocator, vulkan_module);
    mod.load_module(_allocator, imgui_module);
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
            ice::Constant_TraitName_DevUI
        };

        engine.worlds().create_world(
            { .name = "world"_sid, .traits = traits }
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

auto TestGame::rendergraph(ice::gfx::GfxContext& context) noexcept -> ice::UniquePtr<ice::gfx::GfxGraphRuntime>
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
            {.name = "copy"_sid, .outputs = { &fb, 1 }}
        };
        GfxGraphPass const pass1{ .name = "test1"_sid, .stages = stages1 };
        GfxGraphPass const pass2{ .name = "test2"_sid, .stages = stages2 };
        _graph->add_pass(pass1);
        _graph->add_pass(pass2);
    }

    return create_graph_runtime(_allocator, context, *_graph);
}
