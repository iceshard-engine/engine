#include <ice/memory/memory_globals.hxx>
#include <ice/memory/proxy_allocator.hxx>

#include <fmt/format.h>

#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

int main(int argc, char* argv[])
{
    int result = 0;

    {
        ice::memory::init();

        {
            auto& stats = static_cast<ice::memory::ProxyAllocator&>(ice::memory::default_allocator());

            result = Catch::Session().run(argc, argv);

            fmt::print(stderr, "> Statistics:\n");
            fmt::print(stderr, "- total number of allocations: {}\n", stats.allocation_count());
        }

        ice::memory::shutdown();
    }

    return result;
}
