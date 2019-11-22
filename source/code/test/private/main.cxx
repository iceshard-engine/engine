#include <core/memory.hxx>
#include <core/allocators/proxy_allocator.hxx>
#include <core/string.hxx>
#include <core/stack_string.hxx>
#include <core/string_view.hxx>
#include <core/pod/array.hxx>
#include <core/datetime/datetime.hxx>

#include <core/cexpr/stringid.hxx>
#include <core/scope_exit.hxx>
#include <core/debug/profiler.hxx>
#include <core/pod/hash.hxx>

#include <resource/uri.hxx>
#include <resource/system.hxx>
#include <resource/modules/dynlib_module.hxx>
#include <resource/modules/filesystem_module.hxx>

#include <core/message/buffer.hxx>
#include <core/message/operations.hxx>

#include <input_system/module.hxx>
#include <input_system/message/app.hxx>
#include <input_system/message/mouse.hxx>

#include <fmt/format.h>
#include <application/application.hxx>

#include <iceshard/module.hxx>
#include <iceshard/engine.hxx>
#include <iceshard/frame.hxx>
#include <iceshard/world/world.hxx>
#include <iceshard/entity/entity_index.hxx>
#include <iceshard/entity/entity_command_buffer.hxx>
#include <iceshard/component/component_system.hxx>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

class TestComponent1 : public iceshard::ComponentSystem
{
public:
    auto name() const noexcept -> core::cexpr::stringid_type override
    {
        static constexpr auto component_id = core::cexpr::stringid_cexpr("TestComponent1");
        return component_id;
    }

    void create(iceshard::entity_handle_type, core::cexpr::stringid_argument_type component_name) noexcept
    {
        fmt::print("Test1 : create {}!\n", component_name);
    }

    void remove(iceshard::entity_handle_type, core::cexpr::stringid_argument_type component_name) noexcept
    {
        fmt::print("Test1 : remove {}!\n", component_name);
    }

    void test1()
    {
        fmt::print("Test 1 system!\n");
    }
};

class TestComponent2 : public iceshard::ComponentSystem
{
public:
    auto name() const noexcept -> core::cexpr::stringid_type override
    {
        static constexpr auto component_id = core::cexpr::stringid_cexpr("TestComponent2");
        return component_id;
    }

    void create(iceshard::entity_handle_type, core::cexpr::stringid_argument_type component_name) noexcept
    {
        fmt::print("Test2 : create {}!\n", component_name);
    }

    void remove(iceshard::entity_handle_type, core::cexpr::stringid_argument_type component_name) noexcept
    {
        fmt::print("Test2 : remove {}!\n", component_name);
    }

    void test2()
    {
        fmt::print("Test 2 system!\n");
    }
};

int game_main(core::allocator& alloc, resource::ResourceSystem& resources)
{
    using resource::URN;
    using resource::URI;

    resources.mount(URI{ resource::scheme_dynlib, "bin" });
    resources.mount(URI{ resource::scheme_directory, "../source/data/second" });

    auto* engine_module_location = resources.find(URN{ "iceshard.dll" });
    IS_ASSERT(engine_module_location != nullptr, "Missing engine module!");

    if (auto engine_module = iceshard::load_engine_module(alloc, engine_module_location->location().path, resources))
    {
        auto* engine_instance = engine_module->engine();

        fmt::print("IceShard engine revision: {}\n", engine_instance->revision());

        // Create a test world
        engine_instance->world_manager()->create_world(core::cexpr::stringid("test-world"));

        TestComponent1 t1c;
        TestComponent2 t2c;

        iceshard::EntityIndex entity_index{ alloc };
        iceshard::EntityCommandBuffer command_buffer{ alloc };

        int count = 10;

        bool quit = false;
        while (quit == false)
        {

            if (count-- > 0)
            {
                engine_instance->create_long_task([](iceshard::Engine& engine)->cppcoro::task<> {
                    fmt::print("Sleeping for 3s on thread {}!\n", GetCurrentThreadId());
                    SleepEx(3000, 0);

                    engine.create_task([](iceshard::Engine&) -> cppcoro::task<> {
                        fmt::print("Waked up on thread {}!\n", GetCurrentThreadId());
                        co_return;
                    });
                    co_return;
                });
            }

            for (int i = 0; i < 100; ++i)
            {
                engine_instance->create_task([&](iceshard::Engine& engine) noexcept->cppcoro::task<>
                    {
                        //SleepEx(1, 0);
                        co_return;
                    });
            }

            engine_instance->create_task([&](iceshard::Engine& engine) noexcept -> cppcoro::task<>
                {
                    auto& frame = engine.current_frame();

                    // Check for the quit message
                    core::message::filter<input::message::AppExit>(frame.messages(), [&quit](const auto&) noexcept
                        {
                            quit = true;
                        });



                    //[[maybe_unused]]
                    //auto* test_world = engine.world_manager()->get_world(core::cexpr::stringid("test-world"));
                    //auto* services = test_world->service_provider();
                    //auto* entities = services->entity_manager();

                    //auto e = entities->create();

                    //command_buffer.add_component(e, &t1c, core::cexpr::stringid("test1"));
                    //command_buffer.add_component(e, &t2c, core::cexpr::stringid("test2"));
                    //command_buffer.add_component(e, &t1c, core::cexpr::stringid("test3"));

                    //command_buffer.update_component(e, core::cexpr::stringid("test1"), [](iceshard::ComponentSystem* cs, iceshard::entity_handle_type) noexcept
                    //    {
                    //        static_cast<TestComponent1*>(cs)->test1();
                    //        return false;
                    //    });
                    //command_buffer.update_component(e, core::cexpr::stringid("test2"), [](iceshard::ComponentSystem* cs, iceshard::entity_handle_type) noexcept
                    //    {
                    //        static_cast<TestComponent2*>(cs)->test2();
                    //        return false;
                    //    });
                    //command_buffer.update_component(e, core::cexpr::stringid("test3"), [](iceshard::ComponentSystem* cs, iceshard::entity_handle_type) noexcept
                    //    {
                    //        static_cast<TestComponent1*>(cs)->test1();
                    //        return false;
                    //    });

                    //command_buffer.remove_component(e, core::cexpr::stringid("test3"));
                    //command_buffer.remove_component(e, core::cexpr::stringid("test2"));
                    //command_buffer.remove_component(e, core::cexpr::stringid("test1"));

                    //entities->destroy(e);
                    co_return;
                });

            // Update the engine state.
            engine_instance->next_frame();

            //command_buffer.execute(engine_instance->entity_manager(), &entity_index);
        }

        // Destroy the test world
        engine_instance->world_manager()->destroy_world(core::cexpr::stringid("test-world"));
    }

    return 0;
}
