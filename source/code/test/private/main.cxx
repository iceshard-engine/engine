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
#include <resource/filesystem_module.hxx>

#include <core/message/buffer.hxx>
#include <core/message/operations.hxx>

#include <input_system/module.hxx>
#include <input_system/message/app.hxx>

#include <fmt/format.h>

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

int main()
{
    core::memory::globals::init_with_stats();

    auto& alloc = static_cast<core::memory::proxy_allocator&>(core::memory::globals::default_allocator());

    {
        using resource::URN;
        using resource::URI;

        core::StackString<64> config_directory{ "bin/" };
        config_directory += "x64-";
        config_directory += to_string(core::build::configuration::current_config);
        config_directory += "/sdl2_driver/sdl2_driver.dll";

        if (auto driver_module = input::load_driver_module(alloc, config_directory))
        {
            core::MessageBuffer messages{ alloc };

            auto* media_driver = driver_module->media_driver();

            bool quit = false;
            while (quit == false)
            {
                core::message::clear(messages);

                // Get all messages
                media_driver->query_messages(messages);

                // Check for the quit message
                core::message::filter<input::message::AppExit>(messages, [&quit](const auto&) noexcept
                    {
                        quit = true;
                    });
            }
        }

    }

    core::memory::globals::shutdown();
    return 0;
}
