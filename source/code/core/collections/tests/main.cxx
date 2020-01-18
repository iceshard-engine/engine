#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>
#include <core/memory.hxx>
#include <core/string.hxx>
#include <core/allocators/proxy_allocator.hxx>
#include <fmt/format.h>

int main(int argc, char* argv[])
{
    int result = 0;

    {
        core::memory::globals::init_with_stats();

        {
            auto& stats = static_cast<core::memory::proxy_allocator&>(core::memory::globals::default_allocator());

            result = Catch::Session().run(argc, argv);

            fmt::print(stderr, "> Statistics:\n");
            fmt::print(stderr, "- total number of allocations: {}\n", stats.allocation_count());
        }

        core::memory::globals::shutdown();
    }

    return result;
}