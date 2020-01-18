#include <catch2/catch.hpp>
#include <core/memory.hxx>
#include <core/allocators/proxy_allocator.hxx>
#include <core/pod/queue.hxx>

SCENARIO("core :: queue")
{
    namespace queue = core::pod::queue;

    auto alloc = core::memory::proxy_allocator{ "queue_test", core::memory::globals::default_allocator() };
    auto test_queue = core::pod::Queue<int>{ alloc };

    CHECK(queue::size(test_queue) == 0);

    GIVEN("An empty Queue")
    {
        CHECK(queue::size(test_queue) == 0);

        WHEN("Pushing an element")
        {
            queue::push_back(test_queue, 0xd00b);
            CHECK(queue::size(test_queue) == 1);
        }

        WHEN("Pushing 100 elements")
        {
            for (int i = 0; i < 100; ++i)
            {
                queue::push_back(test_queue, 0xd00b);
            }

            CHECK(queue::size(test_queue) == 100);

            THEN("Poping 50 elements")
            {
                for (int i = 0; i < 50; ++i)
                {
                    queue::pop_back(test_queue);
                }

                CHECK(queue::size(test_queue) == 50);
            }

            THEN("Consuming all elements")
            {
                queue::consume(test_queue, queue::size(test_queue));

                CHECK(queue::size(test_queue) == 0);
            }
        }
    }

    GIVEN("A wrapped queue")
    {
        // Reserve space for 10 elements
        queue::reserve(test_queue, 10);
        CHECK(queue::size(test_queue) == 0);

        // Push 7 elements so we move the offset.
        queue::push(test_queue, { 1, 2, 3, 4, 5, 6, 7 });
        CHECK(queue::size(test_queue) == 7);

        // Consume the elements
        queue::consume(test_queue, 7);
        CHECK(queue::size(test_queue) == 0);

        // Push another 6 elements so we got 3 elements at the end of the ring buffer and 3 at the begining
        int test_number_array[] = { 1, 2, 3, 4, 5, 6 };

        queue::push(test_queue, test_number_array);
        CHECK(queue::size(test_queue) == 6);

        THEN("Check if we iterate in the proper order over the queue")
        {
            auto queue_size = queue::size(test_queue);
            for (uint32_t i = 0; i < queue_size; ++i)
            {
                CHECK(test_queue[i] == test_number_array[i]);
            }

            WHEN("Resizing the queue capacity check again")
            {
                auto alloc_count = alloc.allocation_count();
                queue::reserve(test_queue, 100);

                // Check that we did force a reallocation
                CHECK(alloc_count + 1 == alloc.allocation_count());

                // Check the queue is still in tact
                CHECK(queue_size == queue::size(test_queue));
                for (uint32_t i = 0; i < queue_size; ++i)
                {
                    CHECK(test_queue[i] == test_number_array[i]);
                }
            }
        }

        THEN("Check the chunk iterators seeing only the three last elements")
        {
            auto* it = queue::begin_front(test_queue);
            auto* end = queue::end_front(test_queue);

            int index = 0;
            while (it != end)
            {
                CHECK(*it == test_number_array[index++]);
                it++;
            }

            CHECK(index == 3);

            WHEN("Resizing the queue we see all 6 elements now")
            {
                auto alloc_count = alloc.allocation_count();
                queue::reserve(test_queue, 100);

                // Check that we did force a reallocation
                CHECK(alloc_count + 1 == alloc.allocation_count());

                // Get the new iterators values
                it = queue::begin_front(test_queue);
                end = queue::end_front(test_queue);

                // Check the queue is still intact
                index = 0;
                while (it != end)
                {
                    CHECK(*it == test_number_array[index++]);
                    it++;
                }

                CHECK(index == 3);
            }
        }
    }
}
