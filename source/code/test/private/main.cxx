#include <core/memory.hxx>
#include <core/allocators/proxy_allocator.hxx>
#include <core/string.hxx>
#include <core/stack_string.hxx>
#include <core/pod/array.hxx>

#include <core/cexpr/stringid.hxx>
#include <core/scope_exit.hxx>
#include <core/debug/profiler.hxx>

#include <filesystem/uri.hxx>

#include <fmt/format.h>

using core::cexpr::stringid;
using core::cexpr::stringid_invalid;

int main()
{
    core::debug::initialize_profiling_context();

    {
        core::debug::Profiler app_profiler{ "app" };

        {
            core::debug::Profiler profiler_profiler{ "app::profiler" };
        }

        using namespace core::pod::array;

        core::memory::globals::init();

        [[maybe_unused]]
        auto& a = core::memory::globals::default_allocator();
        [[maybe_unused]]
        auto pa = core::memory::proxy_allocator{ "pa", a };

        [[maybe_unused]]
        auto& sa = core::memory::globals::default_scratch_allocator();
        [[maybe_unused]]
        auto psa = core::memory::proxy_allocator{ "psa", sa };

        {
            filesystem::URI uri{ stringid("file"), stringid("test.txt"), { psa, "some/location" } };
            fmt::print("{}\n", uri);
        }

        auto& alloc = psa;

        constexpr auto sid2 = core::cexpr::stringid_cexpr("test2");
        auto sid = core::cexpr::stringid("test1");

        {
            core::debug::Profiler alloc_profiler{ "app::alloc" };
            auto* p = alloc.allocate(100);
            alloc.deallocate(p);
        }

        fmt::print("allocs::global: {}\n", pa.allocation_count());
        fmt::print("allocs::scratch: {}\n", psa.allocation_count());

        core::memory::globals::shutdown();
    }

    core::debug::save_all_profiling_contexts();
    core::debug::clear_profiling_context();

    fmt::print("Profiling events:\n");
    core::debug::visit_all_profiling_contexts(nullptr, [](void*, int, const core::debug::detail::profiler_event& ev) noexcept
        {
            fmt::print("> event: {}\n", ev.event_name);
            fmt::print("- duration: {}us\n", std::chrono::duration_cast<std::chrono::microseconds>(ev.event_end - ev.event_begin).count());
        });

    core::debug::release_profiling_context();
    return 0;
}
