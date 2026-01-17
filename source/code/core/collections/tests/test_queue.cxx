/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <catch2/catch_test_macros.hpp>
#include <ice/mem_allocator_host.hxx>
#include <ice/mem_allocator_proxy.hxx>
#include <ice/queue.hxx>
#include "util_tracking_object.hxx"

SCENARIO("collections 'ice/container/queue.hxx'", "[collection][queue][complex]")
{
    namespace queue = ice::queue;

    ice::HostAllocator host_alloc{ };
    ice::ProxyAllocator alloc{ host_alloc, "queue_test" };
    ice::Queue<Test_TrackingObject, ice::ContainerLogic::Complex> test_queue{ alloc };

    CHECK(test_queue.size() == 0);

    GIVEN("an empty queue")
    {
        WHEN("resizing constructs objects")
        {
            test_queue.resize(5);

            ice::u32 dtor_count = 0;
            Test_ObjectEvents events{};
            for (ice::u32 idx = 0; idx < test_queue.size(); ++idx)
            {
                test_queue[idx].gather_ctors(events);
                test_queue[idx].data.test_dtor = &dtor_count;
            }

            CHECK(events == Test_ObjectEvents{ .test_ctor = 5 });

            WHEN("cleared calls destructors")
            {
                test_queue.clear();

                CHECK(dtor_count == 5);
            }
        }

        WHEN("pushing a value move constructs an object")
        {
            {
                test_queue.push_back({ 24 });
                Test_TrackingObject& test_object = test_queue.front();

                CHECK(test_object == Test_ObjectEvents{ .test_ctor_move = 1 });
                CHECK(test_object.value == 24);
            }

            {
                test_queue.push_front({ 33 });
                Test_TrackingObject& test_object = test_queue.front();

                CHECK(test_object == Test_ObjectEvents{ .test_ctor_move = 1 });
                CHECK(test_object.value == 33);
            }

            {
                test_queue.push_back({ 42 });
                Test_TrackingObject& test_object = test_queue.back();

                CHECK(test_object == Test_ObjectEvents{ .test_ctor_move = 1 });
                CHECK(test_object.value == 42);
            }

            ice::u32 dtor_count = 0;
            Test_ObjectEvents events{};
            for (ice::u32 idx = 0; idx < test_queue.size(); ++idx)
            {
                test_queue[idx].gather_ctors(events);
                test_queue[idx].data.test_dtor = &dtor_count;
            }

            CHECK(events == Test_ObjectEvents{ .test_ctor_move = 3 });

            WHEN("cleared calls destructors")
            {
                test_queue.clear();

                CHECK(dtor_count == 3);
            }
        }
    }

    GIVEN("an wrapped queue")
    {
        test_queue.resize(7);

        ice::u32 dtor_count = 0;
        for (ice::u32 idx = 0; idx < test_queue.size(); ++idx)
        {
            test_queue[idx].data.test_dtor = &dtor_count;
        }

        test_queue.pop_front(6); // We pop 6, as poping everything will reset the queue.

        CHECK(test_queue.size() == 1);
        CHECK(test_queue.capacity() == 7);

        for (Test_TrackingObject obj : { Test_TrackingObject{ 1 }, Test_TrackingObject{ 2 }, Test_TrackingObject{ 3 } })
        {
            test_queue.push_back(obj);
        }
        CHECK(test_queue.size() == 4);

        // Ensure we have a wrapped queue.
        CHECK(test_queue._offset != 0);
        CHECK(test_queue._capacity < (test_queue._offset + test_queue._count));

        for (ice::u32 idx = 0; idx < test_queue.size(); ++idx)
        {
            test_queue[idx].data.test_dtor = &dtor_count;
        }

        // Clear the queue and calculate the dtor count;
        WHEN("cleared calls destructors")
        {
            test_queue.clear();

            CHECK(dtor_count == 10);
        }
    }
}

SCENARIO("collections 'ice/container/queue.hxx' (POD)", "[collection][queue][pod]")
{
    namespace queue = ice::queue;

    ice::HostAllocator host_alloc{ };
    ice::ProxyAllocator alloc{ host_alloc, "queue_test" };
    ice::Queue<ice::i32> test_queue{ alloc };

    CHECK(test_queue.size() == 0);

    GIVEN("An empty queue object")
    {
        CHECK(test_queue.size() == 0);

        WHEN("we can push elements from the front and back")
        {
            test_queue.push_back(0xd00b);
            CHECK(test_queue.size() == 1);

            CHECK(test_queue.front() == 0xd00b);
            CHECK(test_queue.back() == 0xd00b);

            test_queue.push_front(0x0db0);
            CHECK(test_queue.size() == 2);

            CHECK(test_queue.front() == 0x0db0);
            CHECK(test_queue.back() == 0xd00b);

            AND_WHEN("we pop them 'back' then 'front' the queue ends up empty")
            {
                test_queue.pop_back(1);
                CHECK(test_queue.size() == 1);

                CHECK(test_queue.front() == 0x0db0);
                CHECK(test_queue.back() == 0x0db0);

                test_queue.pop_front(1);
                CHECK(test_queue.size() == 0);
                CHECK(test_queue.is_empty());
            }

            AND_WHEN("we pop them 'front' then 'back' the queue ends up empty")
            {
                test_queue.pop_front(1);
                CHECK(test_queue.size() == 1);

                CHECK(test_queue.front() == 0xd00b);
                CHECK(test_queue.back() == 0xd00b);

                test_queue.pop_back(1);
                CHECK(test_queue.size() == 0);
                CHECK(test_queue.is_empty());
            }
        }

        WHEN("we push 100 elements")
        {
            for (ice::u32 i = 0; i < 100; ++i)
            {
                test_queue.push_back(0xd00b);
            }

            CHECK(test_queue.size() == 100);

            AND_WHEN("we create a copy of the array")
            {
                ice::Queue test_copy = test_queue;

                THEN("popping 50 front elements 'front' at once or one-by-one results in the same queue")
                {
                    test_queue.pop_front(50);
                    for (ice::u32 idx = 0; idx < 50; ++idx)
                    {
                        test_copy.pop_front();
                    }

                    for (ice::u32 idx = 0; idx < test_copy.size(); ++idx)
                    {
                        CHECK(test_queue[idx] == test_copy[idx]);
                    }
                }

                THEN("popping from back 50 elements at once or one-by-one results in the same queue")
                {
                    test_queue.pop_back(50);
                    for (ice::u32 idx = 0; idx < 50; ++idx)
                    {
                        test_copy.pop_back();
                    }

                    for (ice::u32 idx = 0; idx < test_copy.size(); ++idx)
                    {
                        CHECK(test_queue[idx] == test_copy[idx]);
                    }
                }
            }

            THEN("we clear the queue by popping all elements (back)")
            {
                test_queue.pop_back(test_queue.size().u32());

                CHECK(test_queue.size() == 0);
            }

            THEN("we clear the queue by popping all elements (front)")
            {
                test_queue.pop_front(test_queue.size().u32());

                CHECK(test_queue.size() == 0);
            }

            THEN("we clear the queue by calling 'clear'")
            {
                test_queue.clear();

                CHECK(test_queue.size() == 0);
            }
        }
    }

    GIVEN("A queue with 7 elements")
    {
        // Reserve space for 10 elements
        test_queue.reserve(10);
        CHECK(test_queue.size() == 0);

        ice::i32 const test_values[]{ 1, 2, 3, 4, 5, 6, 7 };
        test_queue.push_back(ice::Span{ test_values });

        THEN("we preare it so the values are wrapped around the buffer")
        {
            ice::i32 const test_values2[]{ 7, 1, 2, 3, 4, 5, 6, 7 };

            test_queue.pop_front(6); // We pop 6, as poping everything will reset the queue.
            CHECK(test_queue.size() == 1);

            test_queue.push_back(ice::Span{ test_values });
            CHECK(test_queue.size() == 8);

            // Ensure we have a wrapped queue.
            CHECK(test_queue._offset != 0);
            CHECK(test_queue._capacity < (test_queue._offset + test_queue._count));

            AND_THEN("the queue matches test values2")
            {
                for (ice::u32 idx = 0; idx < test_queue.size(); ++idx)
                {
                    CHECK(test_values2[idx] == test_queue[idx]);
                }
            }

            WHEN("a copy is created")
            {
                ice::Queue test_copy = test_queue;

                THEN("the copy is not wrapped but has the same item order")
                {
                    CHECK(test_copy._offset == 0);
                    CHECK(test_queue._offset != 0);
                    CHECK(test_copy.size() == test_queue.size());

                    for (ice::u32 idx = 0; idx < test_copy.size(); ++idx)
                    {
                        CHECK(test_queue[idx] == test_copy[idx]);
                    }
                }
            }

            WHEN("cleared it resets the interall offset")
            {
                test_queue.clear();

                CHECK(test_queue._offset == 0);
                CHECK(test_queue.is_empty());
            }
        }

        test_queue.clear();
        test_queue.push_back(ice::Span{ test_values });

        REQUIRE(test_queue.size() == 7);

        // Popping 6 front elements
        test_queue.pop_front(6);

        // Push another 5 elements so we got 3 elements at the end of the ring buffer and 3 at the begining
        ice::i32 const test_values_2[]{ 7, 1, 2, 3, 4, 5 };

        test_queue.push_back(ice::Span{ test_values_2 + 1, 5 });
        CHECK(test_queue.size() == 6);

        THEN("Check if we iterate in the proper order over the queue")
        {
            ice::ncount const queue_size = test_queue.size();
            for (ice::nindex i = 0; i < queue_size; ++i)
            {
                CHECK(test_queue[i] == test_values_2[i]);
            }

            WHEN("using 'for_each' we iterate as expected in succession")
            {
                ice::u32 idx = 0;
                test_queue.for_each(
                    [&test_values_2, &idx](ice::i32 val) noexcept
                    {
                        CHECK(val == test_values_2[idx]);
                        idx += 1;
                    }
                );
            }

            WHEN("using 'for_each_reverse' we iterate as expected in reverse")
            {
                ice::u32 idx = ice::count(test_values_2) - 1;
                test_queue.for_each_reverse(
                    [&test_values_2, &idx](ice::i32 val) noexcept
                    {
                        CHECK(val == test_values_2[idx]);
                        idx -= 1;
                    }
                );
            }

            WHEN("Resizing the queue capacity check again")
            {
                if constexpr (ice::Allocator::HasDebugInformation)
                {
                    ice::u32 const alloc_count = alloc.allocation_total_count();

                    test_queue.reserve(100);

                    // Check that we did force a reallocation
                    CHECK(alloc_count + 1 == alloc.allocation_total_count());
                }
                else
                {
                    test_queue.reserve(100);
                }

                // Check the queue is still in tact
                CHECK(queue_size == test_queue.size());
                for (ice::u32 i = 0; i < queue_size; ++i)
                {
                    CHECK(test_queue[i] == test_values_2[i]);
                }
            }
        }
    }
}
