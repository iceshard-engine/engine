//#include <catch2/catch.hpp>
//#include <ice/mem_allocator_host.hxx>
//#include <ice/mem_allocator_proxy.hxx>
//#include <ice/pod/queue.hxx>
//
//SCENARIO("ice :: pod :: Queue")
//{
//    namespace queue = ice::pod::queue;
//
//    ice::HostAllocator host_alloc{ };
//    ice::ProxyAllocator alloc{ host_alloc, u8"queue_test" };
//    ice::pod::Queue<int32_t> test_queue{ alloc };
//
//    CHECK(queue::size(test_queue) == 0);
//
//    GIVEN("An empty Queue")
//    {
//        CHECK(queue::size(test_queue) == 0);
//
//        WHEN("Pushing an element")
//        {
//            queue::push_back(test_queue, 0xd00b);
//            CHECK(queue::size(test_queue) == 1);
//        }
//
//        WHEN("Pushing 100 elements")
//        {
//            for (int i = 0; i < 100; ++i)
//            {
//                queue::push_back(test_queue, 0xd00b);
//            }
//
//            CHECK(queue::size(test_queue) == 100);
//
//            THEN("Poping 50 elements")
//            {
//                for (int i = 0; i < 50; ++i)
//                {
//                    queue::pop_back(test_queue);
//                }
//
//                CHECK(queue::size(test_queue) == 50);
//            }
//
//            THEN("Consuming all elements")
//            {
//                queue::consume(test_queue, queue::size(test_queue));
//
//                CHECK(queue::size(test_queue) == 0);
//            }
//        }
//    }
//
//    GIVEN("A wrapped queue")
//    {
//        // Reserve space for 10 elements
//        queue::reserve(test_queue, 10);
//        CHECK(queue::size(test_queue) == 0);
//
//        int32_t const test_values[]{ 1, 2, 3, 4, 5, 6, 7 };
//
//        // Push 7 elements so we move the offset.
//        queue::push_back(test_queue, { test_values });
//        CHECK(queue::size(test_queue) == 7);
//
//        // Consume the elements
//        queue::consume(test_queue, 7);
//        CHECK(queue::size(test_queue) == 0);
//
//        // Push another 6 elements so we got 3 elements at the end of the ring buffer and 3 at the begining
//        int32_t const test_values_2[]{ 1, 2, 3, 4, 5, 6 };
//
//        queue::push_back(test_queue, { test_values_2 });
//        CHECK(queue::size(test_queue) == 6);
//
//        THEN("Check if we iterate in the proper order over the queue")
//        {
//            uint32_t const queue_size = queue::size(test_queue);
//            for (uint32_t i = 0; i < queue_size; ++i)
//            {
//                CHECK(test_queue[i] == test_values_2[i]);
//            }
//
//            WHEN("Resizing the queue capacity check again")
//            {
//                if constexpr (ice::Allocator::HasDebugInformation)
//                {
//                    uint32_t const alloc_count = alloc.allocation_total_count();
//
//                    queue::reserve(test_queue, 100);
//
//                    // Check that we did force a reallocation
//                    CHECK(alloc_count + 1 == alloc.allocation_total_count());
//                }
//                else
//                {
//                    queue::reserve(test_queue, 100);
//                }
//
//                // Check the queue is still in tact
//                CHECK(queue_size == queue::size(test_queue));
//                for (uint32_t i = 0; i < queue_size; ++i)
//                {
//                    CHECK(test_queue[i] == test_values_2[i]);
//                }
//            }
//        }
//
//        //THEN("Check the chunk iterators seeing only the three last elements")
//        //{
//        //    auto* it = queue::begin_front(test_queue);
//        //    auto* end = queue::end_front(test_queue);
//
//        //    int index = 0;
//        //    while (it != end)
//        //    {
//        //        CHECK(*it == test_number_array[index++]);
//        //        it++;
//        //    }
//
//        //    CHECK(index == 3);
//
//        //    WHEN("Resizing the queue we see all 6 elements now")
//        //    {
//        //        auto alloc_count = alloc.allocation_count();
//        //        queue::reserve(test_queue, 100);
//
//        //        // Check that we did force a reallocation
//        //        CHECK(alloc_count + 1 == alloc.allocation_count());
//
//        //        // Get the new iterators values
//        //        it = queue::begin_front(test_queue);
//        //        end = queue::end_front(test_queue);
//
//        //        // Check the queue is still intact
//        //        index = 0;
//        //        while (it != end)
//        //        {
//        //            CHECK(*it == test_number_array[index++]);
//        //            it++;
//        //        }
//
//        //        CHECK(index == 3);
//        //    }
//        //}
//    }
//}
