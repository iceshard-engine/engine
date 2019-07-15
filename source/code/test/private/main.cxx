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
//
//namespace input::message
//{
//    template<> struct MessageInfo<TestFileRequest>
//    {
//        static constexpr std::string_view Name = "TestFileRequest";
//        static constexpr auto ID = core::cexpr::stringid_cexpr("TestFileRequest");
//    };
//    template<> struct MessageInfo<TestFileRequest2>
//    {
//        static constexpr std::string_view Name = "TestFileRequest2";
//        static constexpr auto ID = core::cexpr::stringid_cexpr("TestFileRequest2");
//    };
//}


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

        [[maybe_unused]]
        core::MessageBuffer messages{ alloc };

        for (const auto& msg : messages)
        {
            fmt::print("Message type: {}", msg.header.type);
        }
    }

    core::memory::globals::shutdown();

    system("pause");
    return 0;
}
