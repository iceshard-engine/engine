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

        bool quit = false;
        while (quit == false)
        {
            auto& current_frame = engine_instance->current_frame();

            // Check for the quit message
            core::message::filter<input::message::AppExit>(current_frame.messages(), [&quit](const auto&) noexcept
                {
                    quit = true;
                });

            engine_instance->create_task([](core::allocator& frame_alloc) noexcept -> cppcoro::task<>
                {
                    core::String<> hello_world_string{ frame_alloc, "Hello" };
                    hello_world_string += " World!";
                    fmt::print("{}\n", hello_world_string);
                    co_return;
                });

            // Update the engine state.
            engine_instance->next_frame();
        }
    }

    return 0;
}
