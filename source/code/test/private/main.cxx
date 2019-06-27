#include <core/memory.hxx>
#include <core/allocators/proxy_allocator.hxx>
#include <core/string.hxx>
#include <core/stack_string.hxx>
#include <core/pod/array.hxx>

#include <core/cexpr/stringid.hxx>
#include <core/scope_exit.hxx>
#include <core/debug/profiler.hxx>

#include <filesystem/filesystem.hxx>
#include <filesystem/uri.hxx>

#include <fmt/format.h>

using core::cexpr::stringid;
using core::cexpr::stringid_invalid;

int main()
{
    core::memory::globals::init();
    filesystem::init(core::memory::globals::default_allocator(), "./");
    filesystem::mount("../source/data");

    {
        auto& a = core::memory::globals::default_scratch_allocator();

        core::String test{ a, "test" };

        {
            core::String test2{ a, "test2" };
            test = std::move(test2);
        }

        fmt::print("{}\n", test);
    }

    filesystem::shutdown();
    core::memory::globals::shutdown();

    system("pause");
    return 0;
}
