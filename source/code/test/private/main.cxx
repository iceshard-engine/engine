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


int main()
{
    core::memory::globals::init_with_stats();

    auto& alloc = static_cast<core::memory::proxy_allocator&>(core::memory::globals::default_allocator());

    {
        using resource::URN;
        using resource::URI;

        resource::ResourceSystem res_sys{ alloc };
        {
            core::pod::Array<core::cexpr::stringid_type> schemes{ core::memory::globals::default_scratch_allocator() };
            core::pod::array::push_back(schemes, resource::scheme_directory);
            core::pod::array::push_back(schemes, resource::scheme_file);

            res_sys.add_module(core::memory::make_unique<resource::ResourceModule, resource::FileSystem>(alloc, alloc, ".."), schemes);

            core::pod::array::clear(schemes);
            core::pod::array::push_back(schemes, resource::scheme_dynlib);
            res_sys.add_module(core::memory::make_unique<resource::ResourceModule, resource::DynLibSystem>(alloc, alloc), schemes);
        }

        res_sys.mount(URI{ resource::scheme_dynlib, "bin" });
        res_sys.mount(URI{ resource::scheme_directory, "source/data/second" });

        auto* res = res_sys.find(URN{ "sdl2_driver.dll" });
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

    }

    core::memory::globals::shutdown();
    return 0;
}
