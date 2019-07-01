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

#include <fmt/format.h>

using core::cexpr::stringid;
using core::cexpr::stringid_invalid;

int main()
{
    core::memory::globals::init();

    auto& alloc = core::memory::globals::default_allocator();

    {
        using resource::URN;
        using resource::URI;

        resource::ResourceSystem rs{ alloc };

        core::pod::Array<core::cexpr::stringid_type> schemes{ alloc };
        core::pod::array::push_back(schemes, resource::scheme_directory);
        rs.add_module(core::memory::make_unique<resource::ResourceModule, resource::FileSystem>(alloc, alloc, "../source/data"), schemes);

        rs.mount({ resource::scheme_directory, "first" });
        rs.mount({ resource::scheme_directory, "second" });

        auto* r1 = rs.find(URN{ "filesystem.txt" });
        auto* r2 = rs.find(URI{ resource::scheme_directory, "first", URN{ "test/filesystem.txt" } });

        fmt::print("\n");

        if (r1)
        {
            auto file_data = r1->data();
            fmt::print("Resource: {}\n> size: {}\n", r1->location(), file_data.size());
        }
        if (r2)
        {
            auto file_data = r2->data();
            fmt::print("Resource: {}\n> size: {}\n", r2->location(), file_data.size());
        }
    }

    core::memory::globals::shutdown();

    system("pause");
    return 0;
}
