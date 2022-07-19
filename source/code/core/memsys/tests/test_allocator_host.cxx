#include <catch2/catch.hpp>
#include <ice/mem_allocator_host.hxx>
#include "test_utils.hxx"

SCENARIO("memsys 'ice/mem_allocator_host.hxx'", "[allocators]")
{
    using namespace ice;

    ice::HostAllocator host_allocator{ };

    GIVEN("a host allocator...")
    {
        if constexpr (ice::HostAllocator::HasDebugInformation)
        {
            // If test builds fail then this means that HasDebugInfomation is no longer in sync with the debug implementation of BaseAllocator<true>.

            GIVEN("we have debug information...")
            {
                ice::AllocatorDebugInfo const& dbg_info = host_allocator.debug_info();

                CHECK(dbg_info.location().line() == 9);

                CHECK(dbg_info.allocation_count() == 0);
                CHECK(dbg_info.total_allocated() == 0_B);
            }
        }

        THEN("we can allocate memory...")
        {
            ice::alloc_result alloc_res = host_allocator.allocate(12_B);

            CHECK(alloc_res.size == 12_B);
            CHECK(alloc_res.alignment == ice::ualign::b_default);
            CHECK(alloc_res.result != nullptr);

            AND_THEN("we can do it a second time")
            {
                ice::alloc_result alloc_res2 = host_allocator.allocate({ 1_KiB, ice::ualign::b_128 });

                CHECK(alloc_res2.size == 1_KiB);
                CHECK(alloc_res2.alignment == ice::ualign::b_128);
                CHECK(alloc_res2.result != nullptr);

                if constexpr (ice::HostAllocator::HasDebugInformation)
                {
                    CHECK(host_allocator.allocation_count() == 2);
                    CHECK(host_allocator.total_allocated() == 1_KiB + 12_B);
                }

                host_allocator.deallocate(alloc_res2);
            }

            if constexpr (ice::HostAllocator::HasDebugInformation)
            {
                CHECK(host_allocator.allocation_count() == 1);
                CHECK(host_allocator.total_allocated() == 12_B);
            }

            host_allocator.deallocate(alloc_res);

            if constexpr (ice::HostAllocator::HasDebugInformation)
            {
                CHECK(host_allocator.allocation_count() == 0);
                CHECK(host_allocator.total_allocated() == 0_B);
            }
        }
    }
}
