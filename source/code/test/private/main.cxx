#define _CRT_SECURE_NO_WARNINGS
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

#include <dia2.h>

using core::cexpr::stringid;
using core::cexpr::stringid_invalid;

struct TestFileRequest
{
    static const core::cexpr::stringid_type message_type;

    resource::URN name;
    bool load;
};

const core::cexpr::stringid_type TestFileRequest::message_type{ core::cexpr::stringid("TestFileRequest") };

struct TestFileRequest2
{
    static const core::cexpr::stringid_type message_type;

    resource::URI location;
    bool load;
};

const core::cexpr::stringid_type TestFileRequest2::message_type{ core::cexpr::stringid("TestFileRequest2") };

int game_main(core::allocator& alloc, resource::ResourceSystem& resources)
{
    using resource::URN;
    using resource::URI;

    techland.mount(URI{ resource::scheme_features, "*" });
    auto* res = techland.find(URN{ "latest" });

    fmt::print("{}\n", res->data());

    resources.mount(URI{ resource::scheme_dynlib, "bin" });
    resources.mount(URI{ resource::scheme_directory, "../source/data/second" });

    auto* res = resources.find(URN{ "sdl2_driver.dll" });
    IS_ASSERT(res != nullptr, "Missing SDL2 driver module!");

    if (auto driver_module = input::load_driver_module(alloc, res->location().path))
    {
        core::MessageBuffer messages{ alloc };

        auto* input_sys = driver_module->input_system();

        bool quit = false;
        while (quit == false)
        {
            core::message::clear(messages);

            // Get all messages
            input_sys->query_messages(messages);

            // Check for the quit message
            core::message::filter<input::message::AppExit>(messages, [&quit](const auto&) noexcept
                {
                    quit = true;
                });

            // Check for the quit message
            core::message::filter<input::message::MouseMotion>(messages, [](const auto& msg) noexcept
                {
                    fmt::print("Mouse{{ {}:{} }}\n", msg.x, msg.y);
                });
        }
    }

    return 0;
}
