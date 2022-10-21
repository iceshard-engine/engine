#include <catch2/catch.hpp>
#include <ice/mem_allocator_host.hxx>
#include <ice/mem_allocator_proxy.hxx>
#include <ice/container/queue.hxx>
#include "util_tracking_object.hxx"

SCENARIO("collections 'ice/container/queue.hxx'", "[collection][queue][complex]")
{
    namespace queue = ice::queue;

    ice::HostAllocator host_alloc{ };
    ice::ProxyAllocator alloc{ host_alloc, "queue_test" };
    ice::Queue<Test_TrackingObject, ice::ContainerLogic::Complex> test_queue{ alloc };

    CHECK(queue::count(test_queue) == 0);

    GIVEN("an empty queue")
    {
        WHEN("resizing constructs objects")
        {
            queue::resize(test_queue, 5);

            ice::ucount dtor_count = 0;
            Test_ObjectEvents events{};
            for (ice::ucount idx = 0; idx < queue::count(test_queue); ++idx)
            {
                test_queue[idx].gather_ctors(events);
                test_queue[idx].data.test_dtor = &dtor_count;
            }

            CHECK(events == Test_ObjectEvents{ .test_ctor = 5 });

            WHEN("cleared calls destructors")
            {
                queue::clear(test_queue);

                CHECK(dtor_count == 5);
            }
        }

        WHEN("pushing a value move constructs an object")
        {
            {
                queue::push_back(test_queue, { 24 });
                Test_TrackingObject& test_object = queue::front(test_queue);

                CHECK(test_object == Test_ObjectEvents{ .test_ctor_move = 1 });
                CHECK(test_object.value == 24);
            }

            {
                queue::push_front(test_queue, { 33 });
                Test_TrackingObject& test_object = queue::front(test_queue);

                CHECK(test_object == Test_ObjectEvents{ .test_ctor_move = 1 });
                CHECK(test_object.value == 33);
            }

            {
                queue::push_back(test_queue, { 42 });
                Test_TrackingObject& test_object = queue::back(test_queue);

                CHECK(test_object == Test_ObjectEvents{ .test_ctor_move = 1 });
                CHECK(test_object.value == 42);
            }

            ice::ucount dtor_count = 0;
            Test_ObjectEvents events{};
            for (ice::ucount idx = 0; idx < queue::count(test_queue); ++idx)
            {
                test_queue[idx].gather_ctors(events);
                test_queue[idx].data.test_dtor = &dtor_count;
            }

            CHECK(events == Test_ObjectEvents{ .test_ctor_move = 3 });

            WHEN("cleared calls destructors")
            {
                queue::clear(test_queue);

                CHECK(dtor_count == 3);
            }
        }
    }

    GIVEN("an wrapped queue")
    {
        queue::resize(test_queue, 7);

        ice::ucount dtor_count = 0;
        for (ice::ucount idx = 0; idx < queue::count(test_queue); ++idx)
        {
            test_queue[idx].data.test_dtor = &dtor_count;
        }

        queue::pop_front(test_queue, 6); // We pop 6, as poping everything will reset the queue.

        CHECK(queue::count(test_queue) == 1);
        CHECK(queue::capacity(test_queue) == 7);

        for (Test_TrackingObject obj : { Test_TrackingObject{ 1 }, Test_TrackingObject{ 2 }, Test_TrackingObject{ 3 } })
        {
            queue::push_back(test_queue, obj);
        }
        CHECK(queue::count(test_queue) == 4);

        // Ensure we have a wrapped queue.
        CHECK(test_queue._offset != 0);
        CHECK(test_queue._capacity < (test_queue._offset + test_queue._count));

        for (ice::ucount idx = 0; idx < queue::count(test_queue); ++idx)
        {
            test_queue[idx].data.test_dtor = &dtor_count;
        }

        // Clear the queue and calculate the dtor count;
        WHEN("cleared calls destructors")
        {
            queue::clear(test_queue);

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

    CHECK(queue::count(test_queue) == 0);

    GIVEN("An empty queue object")
    {
        CHECK(queue::count(test_queue) == 0);

        WHEN("we can push elements from the front and back")
        {
            queue::push_back(test_queue, 0xd00b);
            CHECK(queue::count(test_queue) == 1);

            CHECK(queue::front(test_queue) == 0xd00b);
            CHECK(queue::back(test_queue) == 0xd00b);

            queue::push_front(test_queue, 0x0db0);
            CHECK(queue::count(test_queue) == 2);

            CHECK(queue::front(test_queue) == 0x0db0);
            CHECK(queue::back(test_queue) == 0xd00b);

            AND_WHEN("we pop them 'back' then 'front' the queue ends up empty")
            {
                queue::pop_back(test_queue, 1);
                CHECK(queue::count(test_queue) == 1);

                CHECK(queue::front(test_queue) == 0x0db0);
                CHECK(queue::back(test_queue) == 0x0db0);

                queue::pop_front(test_queue, 1);
                CHECK(queue::count(test_queue) == 0);
                CHECK(queue::empty(test_queue));
            }

            AND_WHEN("we pop them 'front' then 'back' the queue ends up empty")
            {
                queue::pop_front(test_queue, 1);
                CHECK(queue::count(test_queue) == 1);

                CHECK(queue::front(test_queue) == 0xd00b);
                CHECK(queue::back(test_queue) == 0xd00b);

                queue::pop_back(test_queue, 1);
                CHECK(queue::count(test_queue) == 0);
                CHECK(queue::empty(test_queue));
            }
        }

        WHEN("we push 100 elements")
        {
            for (ice::ucount i = 0; i < 100; ++i)
            {
                queue::push_back(test_queue, 0xd00b);
            }

            CHECK(queue::count(test_queue) == 100);

            AND_WHEN("we create a copy of the array")
            {
                ice::Queue test_copy = test_queue;

                THEN("popping 50 front elements 'front' at once or one-by-one results in the same queue")
                {
                    queue::pop_front(test_queue, 50);
                    for (ice::ucount idx = 0; idx < 50; ++idx)
                    {
                        queue::pop_front(test_copy);
                    }

                    for (ice::ucount idx = 0; idx < ice::queue::count(test_copy); ++idx)
                    {
                        CHECK(test_queue[idx] == test_copy[idx]);
                    }
                }

                THEN("popping from back 50 elements at once or one-by-one results in the same queue")
                {
                    queue::pop_back(test_queue, 50);
                    for (ice::ucount idx = 0; idx < 50; ++idx)
                    {
                        queue::pop_back(test_copy);
                    }

                    for (ice::ucount idx = 0; idx < ice::queue::count(test_copy); ++idx)
                    {
                        CHECK(test_queue[idx] == test_copy[idx]);
                    }
                }
            }

            THEN("we clear the queue by popping all elements (back)")
            {
                queue::pop_back(test_queue, queue::count(test_queue));

                CHECK(queue::count(test_queue) == 0);
            }

            THEN("we clear the queue by popping all elements (front)")
            {
                queue::pop_front(test_queue, queue::count(test_queue));

                CHECK(queue::count(test_queue) == 0);
            }

            THEN("we clear the queue by calling 'clear'")
            {
                queue::clear(test_queue);

                CHECK(queue::count(test_queue) == 0);
            }
        }
    }

    GIVEN("A queue with 7 elements")
    {
        // Reserve space for 10 elements
        queue::reserve(test_queue, 10);
        CHECK(queue::count(test_queue) == 0);

        ice::i32 const test_values[]{ 1, 2, 3, 4, 5, 6, 7 };
        queue::push_back(test_queue, { test_values });

        THEN("we preare it so the values are wrapped around the buffer")
        {
            ice::i32 const test_values2[]{ 7, 1, 2, 3, 4, 5, 6, 7 };

            queue::pop_front(test_queue, 6); // We pop 6, as poping everything will reset the queue.
            CHECK(queue::count(test_queue) == 1);

            queue::push_back(test_queue, { test_values });
            CHECK(queue::count(test_queue) == 8);

            // Ensure we have a wrapped queue.
            CHECK(test_queue._offset != 0);
            CHECK(test_queue._capacity < (test_queue._offset + test_queue._count));

            AND_THEN("the queue matches test values2")
            {
                for (ice::ucount idx = 0; idx < ice::queue::count(test_queue); ++idx)
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
                    CHECK(queue::count(test_copy) == queue::count(test_queue));

                    for (ice::ucount idx = 0; idx < ice::queue::count(test_copy); ++idx)
                    {
                        CHECK(test_queue[idx] == test_copy[idx]);
                    }
                }
            }

            WHEN("cleared it resets the interall offset")
            {
                ice::queue::clear(test_queue);

                CHECK(test_queue._offset == 0);
                CHECK(ice::queue::empty(test_queue));
            }
        }

        queue::clear(test_queue);
        queue::push_back(test_queue, { test_values });

        REQUIRE(queue::count(test_queue) == 7);

        // Popping 6 front elements
        queue::pop_front(test_queue, 6);

        // Push another 5 elements so we got 3 elements at the end of the ring buffer and 3 at the begining
        ice::i32 const test_values_2[]{ 7, 1, 2, 3, 4, 5 };

        queue::push_back(test_queue, { test_values_2 + 1, 5 });
        CHECK(queue::count(test_queue) == 6);

        THEN("Check if we iterate in the proper order over the queue")
        {
            ice::ucount const queue_size = queue::count(test_queue);
            for (ice::ucount i = 0; i < queue_size; ++i)
            {
                CHECK(test_queue[i] == test_values_2[i]);
            }

            WHEN("Resizing the queue capacity check again")
            {
                if constexpr (ice::Allocator::HasDebugInformation)
                {
                    ice::ucount const alloc_count = alloc.allocation_total_count();

                    queue::reserve(test_queue, 100);

                    // Check that we did force a reallocation
                    CHECK(alloc_count + 1 == alloc.allocation_total_count());
                }
                else
                {
                    queue::reserve(test_queue, 100);
                }

                // Check the queue is still in tact
                CHECK(queue_size == queue::count(test_queue));
                for (ice::ucount i = 0; i < queue_size; ++i)
                {
                    CHECK(test_queue[i] == test_values_2[i]);
                }
            }
        }
    }
}
