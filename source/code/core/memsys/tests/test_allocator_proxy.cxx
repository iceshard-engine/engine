#include <catch2/catch.hpp>
#include <ice/memory/memory_globals.hxx>
#include <ice/memory/proxy_allocator.hxx>
#include <ice/memory/stack_allocator.hxx>

TEST_CASE("memory :: proxy allocator", "[allocators]")
{
    ice::memory::init();

    ice::Allocator& backing_alloc = ice::memory::default_allocator();
    uint32_t const initial_backing_total = backing_alloc.total_allocated();

    SECTION("initialization does not allocate anything in the backing allocator")
    {
        ice::memory::ProxyAllocator proxy_alloc{ backing_alloc, "proxy" };

        CHECK(initial_backing_total == backing_alloc.total_allocated());
    }

    GIVEN("an allocation of 120 bytes")
    {
        ice::memory::ProxyAllocator proxy_alloc{ backing_alloc, "proxy" };

        void* test_ptr = proxy_alloc.allocate(120);

        THEN("we track the total allocation")
        {
            CHECK(proxy_alloc.total_allocated() == 120);
        }

        THEN("we deallocate all memory properly")
        {
            proxy_alloc.deallocate(test_ptr);
            test_ptr = nullptr;

            CHECK(backing_alloc.total_allocated() == initial_backing_total);
        }

        THEN("we dont have an overhead due to the proxy")
        {
            uint32_t const backing_total_by_proxy = backing_alloc.total_allocated();
            proxy_alloc.deallocate(test_ptr);

            test_ptr = backing_alloc.allocate(120);
            uint32_t const backing_total_immediate = backing_alloc.total_allocated();

            backing_alloc.deallocate(test_ptr);
            test_ptr = nullptr;

            CHECK(backing_total_by_proxy == backing_total_immediate);
        }

        GIVEN("the backing allocator tracks memory")
        {
            void* test_ptr2 = backing_alloc.allocate(4);
            REQUIRE(backing_alloc.allocated_size(test_ptr2) == 4);

            THEN("The proxy allocator also tracks memory")
            {
                uint32_t const allocation_size = proxy_alloc.allocated_size(test_ptr);
                CHECK(allocation_size == 120);
            }

            backing_alloc.deallocate(test_ptr2);
        }

        if (test_ptr != nullptr)
        {
            proxy_alloc.deallocate(test_ptr);
        }
    }

    ice::memory::shutdown();
}
