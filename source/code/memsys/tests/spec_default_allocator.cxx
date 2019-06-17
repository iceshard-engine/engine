#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>
#include <memsys/memsys.hxx>

SCENARIO("memsys :: default_allocator", "[allocators]")
{
    GIVEN("The global memory system is initialized with a 64 byte scratch allocator")
    {
        memsys::globals::init(64);

        auto& default_alloc = memsys::globals::default_allocator();;
        auto& default_scratch_alloc = memsys::globals::default_scratch_allocator();

        THEN("The scratch allocator allocated memory on the default allocator")
        {
            CHECK(default_alloc.total_allocated() == 64 + 8 /* internal overhead */);
        }

        AND_THEN("We can allocate memory on the scratch allocator")
        {
            void* allocated_memory = default_scratch_alloc.allocate(12);

            AND_THEN("We need to release it!")
            {
                default_scratch_alloc.deallocate(allocated_memory);
            }
        }

        memsys::globals::shutdown();
    }
}
