/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <catch2/catch.hpp>
#include <ice/mem_unique_ptr.hxx>

//#include <ice/memory/memory_globals.hxx>
//#include <ice/memory/proxy_allocator.hxx>
//#include <ice/memory/forward_allocator.hxx>
//
//TEST_CASE("memory :: forward allocator", "[allocators]")
//{
//    ice::memory::init();
//    ice::memory::ProxyAllocator backing_allocator{ ice::memory::default_allocator(), "proxy" };
//
//    uint32_t const initial_allocation_size = backing_allocator.total_allocated();
//
//    SECTION("a initialized forward allocates the first bucket immediately")
//    {
//        ice::memory::ForwardAllocator forward_alloc{ backing_allocator, 1024 };
//
//        CHECK(backing_allocator.total_allocated() == 1024 + 24);
//    }
//
//    SECTION("deallocating does nothing to the backing allocator")
//    {
//        ice::memory::ForwardAllocator forward_alloc{ backing_allocator, 1024 };
//
//        void* ptr1 = forward_alloc.allocate(64);
//        void* ptr2 = forward_alloc.allocate(333);
//        void* ptr3 = forward_alloc.allocate(2048);
//
//        uint32_t const total_allocated = backing_allocator.total_allocated();
//        CHECK(total_allocated >= 1024 * 2);
//
//        forward_alloc.deallocate(ptr1);
//        forward_alloc.deallocate(ptr2);
//        forward_alloc.deallocate(ptr3);
//
//        CHECK(backing_allocator.total_allocated() >= total_allocated);
//    }
//
//    SECTION("relasing all ends with a single allocated bucket + header")
//    {
//        ice::memory::ForwardAllocator forward_alloc{ backing_allocator, 1024 };
//
//        void* ptr1 = forward_alloc.allocate(64);
//        void* ptr2 = forward_alloc.allocate(333);
//        void* ptr3 = forward_alloc.allocate(2048);
//
//        CHECK(backing_allocator.total_allocated() >= 1024 * 2);
//
//        forward_alloc.release_all();
//
//        CHECK(backing_allocator.total_allocated() == 1024 + 24);
//    }
//
//    GIVEN("small bucket size")
//    {
//        ice::memory::ForwardAllocator forward_alloc{ backing_allocator, 16 };
//
//        void* data_4b = forward_alloc.allocate(4);
//        CHECK(backing_allocator.total_allocated() == 16 + 24);
//
//        void* data_8b = forward_alloc.allocate(8);
//        CHECK(backing_allocator.total_allocated() == 16 + 24);
//
//        WHEN("allocation size equals bucket size")
//        {
//            void* data_16b = forward_alloc.allocate(16);
//            CHECK(backing_allocator.total_allocated() == (16 + 20) + 24 * 2);
//        }
//
//        WHEN("allocation size excedes bucket size")
//        {
//            void* data_120b = forward_alloc.allocate(120);
//            CHECK(backing_allocator.total_allocated() == (16 + 124) + 24 * 2);
//        }
//    }
//
//    SECTION("large bucket size")
//    {
//        ice::memory::ForwardAllocator forward_alloc{ backing_allocator, 1024 };
//
//        void* data_4b = forward_alloc.allocate(4);
//        CHECK(backing_allocator.total_allocated() == 1024 + 24);
//
//        void* data_8b = forward_alloc.allocate(8);
//        CHECK(backing_allocator.total_allocated() == 1024 + 24);
//
//        void* data_16b = forward_alloc.allocate(16);
//        CHECK(backing_allocator.total_allocated() == 1024 + 24);
//
//        void* data_120b = forward_alloc.allocate(120);
//        CHECK(backing_allocator.total_allocated() == 1024 + 24);
//    }
//
//    CHECK(backing_allocator.total_allocated() == initial_allocation_size);
//    ice::memory::shutdown();
//}
