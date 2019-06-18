#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>
#include <core/memory.hxx>

SCENARIO("memsys :: default_allocator", "[allocators]")
{
    GIVEN("The global memory system is initialized with a 64 byte scratch allocator")
    {
        core::memory::globals::init(64);

        auto& default_alloc = core::memory::globals::default_allocator();
        auto& default_scratch_alloc = core::memory::globals::default_scratch_allocator();

        THEN("The scratch allocator allocated memory on the default allocator")
        {
            // Internally the allocation header is currently 8 bytes large and has a 4 bytes alignment thus adding a whole 12 bytes to the requested allocation.
            CHECK(default_alloc.total_allocated() == 64 + 12 /* internal overhead */);
        }

        AND_THEN("We can allocate memory on the scratch allocator")
        {
            void* allocated_memory = default_scratch_alloc.allocate(12);

            CHECK(default_scratch_alloc.allocated_size(allocated_memory) == 12);

            AND_THEN("We need to release it!")
            {
                default_scratch_alloc.deallocate(allocated_memory);
            }
        }

        GIVEN("A list of alignments")
        {
            auto test_align = GENERATE(
                std::pair{ 4 /* alignment */, 0x03 /* mask */ }
                , std::pair{ 8 /* alignment */, 0x07 /* mask */ }
                , std::pair{ 16 /* alignment */, 0x0f /* mask */ }
                , std::pair{ 32 /* alignment */, 0x1f /* mask */ }
            );

            THEN("Allcations will be properly aligned")
            {
                void* pointer = default_alloc.allocate(32, test_align.first);

                CHECK(default_alloc.allocated_size(pointer) == 32);

                REQUIRE((reinterpret_cast<uintptr_t>(pointer) & test_align.second) == 0);

                default_alloc.deallocate(pointer);
            }
        }

        core::memory::globals::shutdown();
    }
}
