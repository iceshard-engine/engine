#include <catch2/catch.hpp>
#include <ice/mem_allocator_host.hxx>
#include <ice/mem_allocator_proxy.hxx>
#include "test_utils.hxx"

SCENARIO("memsys 'ice/mem_allocator_proxy.hxx'", "[allocators]")
{
    using namespace ice;

    ice::HostAllocator host_alloc{ };
    ice::ProxyAllocator proxy_alloc{ host_alloc, u8"test_proxy" };

    GIVEN("a proxy allocator...")
    {
        if constexpr (ice::ProxyAllocator::HasDebugInformation)
        {
            // If test builds fail then this means that HasDebugInfomation is no longer in sync with the debug implementation of BaseAllocator<true>.

            GIVEN("we have debug information...")
            {
                ice::AllocatorDebugInfo const& dbg_info = proxy_alloc.debug_info();

                CHECK(dbg_info.parent_allocator() == &host_alloc.debug_info());
                CHECK(host_alloc.debug_info().child_allocator() == &proxy_alloc.debug_info());

                CHECK(proxy_alloc.debug_info().child_allocator() == nullptr);
                CHECK(proxy_alloc.debug_info().next_sibling() == nullptr);

                CHECK(dbg_info.allocation_count() == 0);
                CHECK(dbg_info.allocation_size_inuse() == 0_B);
            }
        }

        THEN("we can allocate memory...")
        {
            ice::alloc_result alloc_res = proxy_alloc.allocate(12_B);

            CHECK(alloc_res.size == 12_B);
            CHECK(alloc_res.alignment == ice::ualign::b_default);
            CHECK(alloc_res.result != nullptr);

            AND_THEN("we can do it a second time")
            {
                ice::alloc_result alloc_res2 = proxy_alloc.allocate({ 1_KiB, ice::ualign::b_128 });

                CHECK(alloc_res2.size == 1_KiB);
                CHECK(alloc_res2.alignment == ice::ualign::b_128);
                CHECK(alloc_res2.result != nullptr);

                if constexpr (ice::HostAllocator::HasDebugInformation)
                {
                    CHECK(proxy_alloc.allocation_count() == 2);
                    CHECK(proxy_alloc.allocation_size_inuse() == 1_KiB + 12_B);
                }

                proxy_alloc.deallocate(alloc_res2);
            }

            if constexpr (ice::HostAllocator::HasDebugInformation)
            {
                CHECK(proxy_alloc.allocation_count() == 1);
                CHECK(proxy_alloc.allocation_size_inuse() == 12_B);
            }

            proxy_alloc.deallocate(alloc_res);

            if constexpr (ice::HostAllocator::HasDebugInformation)
            {
                CHECK(proxy_alloc.allocation_count() == 0);
                CHECK(proxy_alloc.allocation_size_inuse() == 0_B);
            }
        }

        THEN("allocating on the proxy parent does not affect the proxy")
        {
            ice::alloc_result res = host_alloc.allocate(1_KiB);

            if constexpr (ice::HostAllocator::HasDebugInformation)
            {
                CHECK(proxy_alloc.allocation_count() == 0);
                CHECK(proxy_alloc.allocation_size_inuse() == 0_B);

                CHECK(host_alloc.allocation_count() == 1);
                CHECK(host_alloc.allocation_size_inuse() == 1_KiB);
            }

            host_alloc.deallocate(res);
        }
    }
}
