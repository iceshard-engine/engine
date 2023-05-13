/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <catch2/catch_test_macros.hpp>
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

                CHECK(dbg_info.location().line() == 12);

                CHECK(dbg_info.allocation_count() == 0);
                CHECK(dbg_info.allocation_size_inuse() == 0_B);
            }
        }

        THEN("we can allocate memory...")
        {
            ice::AllocResult alloc_res = host_allocator.allocate(12_B);

            CHECK(alloc_res.size == 12_B);
            CHECK(alloc_res.alignment == ice::ualign::b_default);
            CHECK(alloc_res.memory != nullptr);

            AND_THEN("we can do it a second time")
            {
                ice::AllocResult alloc_res2 = host_allocator.allocate({ 1_KiB, ice::ualign::b_128 });

                CHECK(alloc_res2.size == 1_KiB);
                CHECK(alloc_res2.alignment == ice::ualign::b_128);
                CHECK(alloc_res2.memory != nullptr);

                if constexpr (ice::HostAllocator::HasDebugInformation)
                {
                    CHECK(host_allocator.allocation_count() == 2);
                    CHECK(host_allocator.allocation_size_inuse() == 1_KiB + 12_B);
                }

                host_allocator.deallocate(alloc_res2);
            }

            if constexpr (ice::HostAllocator::HasDebugInformation)
            {
                CHECK(host_allocator.allocation_count() == 1);
                CHECK(host_allocator.allocation_size_inuse() == 12_B);
            }

            host_allocator.deallocate(alloc_res);

            if constexpr (ice::HostAllocator::HasDebugInformation)
            {
                CHECK(host_allocator.allocation_count() == 0);
                CHECK(host_allocator.allocation_size_inuse() == 0_B);
            }
        }
    }
}
