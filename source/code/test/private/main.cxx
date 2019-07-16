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

#include <core/message/buffer.hxx>
#include <core/message/operations.hxx>

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

        resource::ResourceSystem rs{ alloc };

        core::pod::Array<core::cexpr::stringid_type> schemes{ alloc };
        core::pod::array::push_back(schemes, resource::scheme_directory);
        rs.add_module(core::memory::make_unique<resource::ResourceModule, resource::FileSystem>(alloc, alloc, "../source/data"), schemes);

        rs.mount({ resource::scheme_directory, "first" });
        rs.mount({ resource::scheme_directory, "second" });

        [[maybe_unused]]
        auto pre_alloc_count = alloc.allocation_count();

        core::MessageBuffer messages{ alloc };

        core::message::push(messages, TestFileRequest{ URN{ "test.txt" }, false });
        core::message::push(messages, TestFileRequest2{ URI{ resource::scheme_directory, "location", URN{ "test.txt" } }, false });

        fmt::print("Allocations made: {}\n", alloc.allocation_count() - pre_alloc_count);

        core::message::filter<TestFileRequest2>(messages, [](const auto& msg) noexcept -> void
            {
                fmt::print("* URI request: {}\n", msg.location);
            });

    }

    core::memory::globals::shutdown();

    system("pause");
    return 0;
}
