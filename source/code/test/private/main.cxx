#include <core/memory.hxx>
#include <core/allocators/proxy_allocator.hxx>
#include <core/string.hxx>
#include <core/stack_string.hxx>
#include <core/string_view.hxx>
#include <core/pod/array.hxx>

#include <core/cexpr/stringid.hxx>
#include <core/scope_exit.hxx>
#include <core/debug/profiler.hxx>

#include <resource/uri.hxx>
#include <resource/system.hxx>
#include <resource/filesystem_module.hxx>

#include <input/utils/message_pipe.h>
#include <input/utils/message_filter.h>
#include <input/iolib.h>

#include <fmt/format.h>

using core::cexpr::stringid;
using core::cexpr::stringid_invalid;

struct TestFileRequest
{
    resource::URN name;
    bool load;
};

struct TestFileRequest2
{
    resource::URI location;
    bool load;
};

namespace input::message
{
    template<> struct MessageInfo<TestFileRequest>
    {
        static constexpr std::string_view Name = "TestFileRequest";
        static constexpr auto ID = core::cexpr::stringid_cexpr("TestFileRequest");
    };
    template<> struct MessageInfo<TestFileRequest2>
    {
        static constexpr std::string_view Name = "TestFileRequest2";
        static constexpr auto ID = core::cexpr::stringid_cexpr("TestFileRequest2");
    };
}

uint32_t input::ticks()
{
    return 0;
}

int main()
{
    core::memory::globals::init_with_stats();

    auto& alloc = static_cast<core::memory::proxy_allocator&>(core::memory::globals::default_allocator());

    {
        using resource::URN;
        using resource::URI;

        resource::ResourceSystem rs{ alloc };

        core::pod::Array<core::cexpr::stringid_type> schemes{ alloc };
        core::pod::array::push_back(schemes, resource::scheme_directory);
        rs.add_module(core::memory::make_unique<resource::ResourceModule, resource::FileSystem>(alloc, alloc, "../source/data"), schemes);

        rs.mount({ resource::scheme_directory, "first" });
        rs.mount({ resource::scheme_directory, "second" });
        /*
                auto* r1 = rs.find(URN{ "test/filesystem.txt" });
                auto* r2 = rs.find(URI{ resource::scheme_directory, "first", URN{ "test/filesystem.txt" } });

        */
        auto pre_alloc_count = alloc.allocation_count();

        input::MessagePipe message_pipe{ alloc };
        input::push(message_pipe, TestFileRequest{ URN{ "filesystem.txt" } });
        input::push(message_pipe, TestFileRequest2{ URI{ resource::scheme_directory, "first", URN{ "filesystem.txt" } } });
        input::push(message_pipe, TestFileRequest{ URN{ "test/filesystem.txt" }, true });
        input::push(message_pipe, TestFileRequest2{ URI{ resource::scheme_directory, "second", URN{ "test/filesystem.txt" } }, true });

        fmt::print("Allocations made: {}\n", alloc.allocation_count() - pre_alloc_count);

        using FnSign = void(void*, const TestFileRequest&, const input::message::Metadata&);
        using FnSign2 = void(void*, const TestFileRequest2&, const input::message::Metadata&);

        FnSign* fn = [](void* ud, const TestFileRequest& msg, const auto&)
        {
            const auto rs = reinterpret_cast<resource::ResourceSystem*>(ud);

            auto* res = rs->find(msg.name);

            bool loaded = false;
            if (res && msg.load == true)
            {
                loaded = res->data().data() != nullptr;
            }

            fmt::print("Resource request: {}\n> found: {}\n> loaded: {}\n", msg.name, res != nullptr, loaded);
        };

        FnSign2* fn2 = [](void* ud, const TestFileRequest2& msg, const auto&)
        {
            const auto rs = reinterpret_cast<resource::ResourceSystem*>(ud);

            auto* res = rs->find(msg.location);

            bool loaded = false;
            if (res && msg.load == true)
            {
                loaded = res->data().data() != nullptr;
            }

            fmt::print("Resource request: {}\n> found: {}\n> loaded: {}\n", msg.location, res != nullptr, loaded);
        };

        input::for_each(message_pipe, &rs, fn);
        input::for_each(message_pipe, &rs, fn2);

    }

    core::memory::globals::shutdown();

    system("pause");
    return 0;
}
