#include <catch2/catch.hpp>
#include <ice/memory/memory_globals.hxx>

SCENARIO("memory :: default allocator", "[allocators]")
{
    GIVEN("a memory system with a 64 byte scratch allocator")
    {
        ice::memory::init(64);

        ice::Allocator& default_alloc = ice::memory::default_allocator();
        ice::Allocator& default_scratch_alloc = ice::memory::default_scratch_allocator();

        THEN("we know the initial allocated memory")
        {
            // Internally the allocation header is currently 8 bytes large and has a 4 bytes alignment thus adding a whole 12 bytes to the requested allocation.
            CHECK(default_alloc.total_allocated() == 64 + 12 /* internal overhead */);
        }

        AND_THEN("allocate memory on the scratch allocator")
        {
            void* allocated_memory = default_scratch_alloc.allocate(12);

            CHECK(default_scratch_alloc.allocated_size(allocated_memory) == 12);
            CHECK(default_scratch_alloc.total_allocated() >= 12);

            AND_THEN("release it!")
            {
                default_scratch_alloc.deallocate(allocated_memory);
            }

            CHECK(default_scratch_alloc.total_allocated() == 0);
        }

        GIVEN("a list of alignments")
        {
            auto test_align = GENERATE(
                std::pair{ 4 /* alignment */, 0x03 /* mask */ }
                , std::pair{ 8 /* alignment */, 0x07 /* mask */ }
                , std::pair{ 16 /* alignment */, 0x0f /* mask */ }
                , std::pair{ 32 /* alignment */, 0x1f /* mask */ }
            );

            THEN("allocations will be properly aligned")
            {
                void* pointer = default_alloc.allocate(32, test_align.first);

                CHECK(default_alloc.allocated_size(pointer) == 32);

                REQUIRE((reinterpret_cast<uintptr_t>(pointer) & test_align.second) == 0);

                default_alloc.deallocate(pointer);
            }
        }

        THEN("we don't have memory leaks")
        {
            // Internally the allocation header is currently 8 bytes large and has a 4 bytes alignment thus adding a whole 12 bytes to the requested allocation.
            CHECK(default_alloc.total_allocated() == 64 + 12 /* internal overhead */);
        }

        ice::memory::shutdown();
    }
}
