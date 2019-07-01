#include <core/memory.hxx>
#include <core/allocators/proxy_allocator.hxx>
#include <core/string.hxx>
#include <core/stack_string.hxx>
#include <core/string_view.hxx>
#include <core/pod/array.hxx>

#include <core/cexpr/stringid.hxx>
#include <core/scope_exit.hxx>
#include <core/debug/profiler.hxx>

#include <filesystem/filesystem.hxx>
#include <resource/uri.hxx>

#include <fmt/format.h>

using core::cexpr::stringid;
using core::cexpr::stringid_invalid;

int main()
{
    core::memory::globals::init();

    auto& alloc = core::memory::globals::default_allocator();

    {
        using resource::URI;
        using resource::URN;

        resource::FileSystem fs{ alloc, "../source/data" };
        fs.mount({ resource::scheme_directory, "first" });
        fs.mount({ resource::scheme_directory, "second" });

        auto* r1 = fs.find(URN{ "filesystem.txt" });
        auto* r2 = fs.find(URI{ resource::scheme_directory, "first", URN{ "test/filesystem.txt" } });

        if (r1) fmt::print("R1: {}\n", r1->location());
        if (r2) fmt::print("R2: {}\n", r2->location());
    }

    core::memory::globals::shutdown();

    system("pause");
    return 0;
}
