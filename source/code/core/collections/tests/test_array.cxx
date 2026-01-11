/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <catch2/catch_test_macros.hpp>
#include <ice/array.hxx>
#include <ice/mem_allocator_host.hxx>
#include <ice/mem_allocator_null.hxx>
#include "util_tracking_object.hxx"

SCENARIO("collections 'ice/container/array.hxx'", "[collection][array][complex]")
{
    ice::HostAllocator alloc;
    ice::Array<Test_TrackingObject, ice::ContainerLogic::Complex> objects{ alloc };

    GIVEN("an empty Array object")
    {
        CHECK(objects.is_empty());
        CHECK(objects.not_empty() == false);
        CHECK(objects.capacity() == 0);
        CHECK(objects.size() == 0);

        // We force a capacity of 1, so we ensure a reallocation in later.
        objects.set_capacity(1);

        WHEN("adding a new object")
        {
            objects.push_back(Test_TrackingObject{ });

            REQUIRE(objects.capacity() >= 1);
            REQUIRE(objects.size() == 1);

            THEN("constructors are called")
            {
                Test_TrackingObject& obj = objects.first();

                CHECK(obj == Test_ObjectEvents{ .test_ctor_move = 1 });

                AND_WHEN("we remove the object the destructor is called")
                {
                    ice::u32 dtor_val = 0;
                    obj.data.test_dtor = &dtor_val;
                    objects.pop_back();

                    CHECK(dtor_val == 1);
                }
            }

            AND_WHEN("resizing the array")
            {
                Test_TrackingObject& obj = objects.first();
                ice::u32 dtor_val = 0;
                obj.data.test_dtor = &dtor_val;

                objects.resize(10);

                // The old object was moved to a new location (move ctor + dtor)
                CHECK(dtor_val == 1);

                CHECK(objects.not_empty());
                CHECK(objects.is_empty() == false);
                CHECK(objects.capacity() >= 10);
                CHECK(objects.size() == 10);

                THEN("constructors are called")
                {
                    Test_TrackingObject& obj_front = objects.first();
                    Test_TrackingObject& obj_back = objects.last();

                    CHECK(obj_front == Test_ObjectEvents{ .test_ctor_move = 1 });
                    CHECK(obj_back == Test_ObjectEvents{ .test_ctor = 1 });
                }

                AND_THEN("we can copy and push back the same elements")
                {
                    ice::Array array_copy = objects;
                    objects.push_back(array_copy);

                    CHECK(objects.capacity() >= 20);
                    CHECK(objects.not_empty());
                    CHECK(objects.is_empty() == false);
                    CHECK(objects.size() == 20);

                    ice::u32 copied_objects = 0;
                    ice::u32 moved_objects = 0;
                    for (Test_TrackingObject const& object : objects)
                    {
                        moved_objects += object.data.test_ctor_move != 0;
                        copied_objects += object.data.test_ctor_copy != 0;
                    }
                    CHECK(moved_objects == 10);
                    CHECK(copied_objects == 10);

                    AND_THEN("clearning it will destroy all objects")
                    {
                        dtor_val = 0;
                        for (Test_TrackingObject& object : objects)
                        {
                            object.data.test_dtor = &dtor_val;
                        }

                        objects.clear();

                        CHECK(dtor_val == 20);
                        CHECK(objects.capacity() > 0);
                        CHECK(objects.is_empty());
                        CHECK(objects.not_empty() == false);
                        CHECK(objects.size() == 0);
                    }
                }

                THEN("moving the array will not affect the objects")
                {
                    dtor_val = 0;
                    for (Test_TrackingObject& object : objects)
                    {
                        object.data.test_dtor = &dtor_val;
                    }

                    Test_ObjectEvents total_events{ };
                    for (Test_TrackingObject const& object : objects)
                    {
                        object.gather_ctors(total_events);
                    }

                    CHECK(total_events == Test_ObjectEvents{ .test_ctor = 9, .test_ctor_move = 1 });

                    ice::Array moved_objects = ice::move(objects);
                    CHECK(dtor_val == 0);

                    CHECK(objects.capacity() == 0);
                    CHECK(objects.is_empty());
                    CHECK(objects.not_empty() == false);
                    CHECK(objects.size() == 0);

                    total_events = Test_ObjectEvents{ };
                    for (Test_TrackingObject const& object : moved_objects)
                    {
                        object.gather_ctors(total_events);
                    }

                    CHECK(total_events == Test_ObjectEvents{ .test_ctor = 9, .test_ctor_move = 1 });
                }
            }
        }
    }
}

SCENARIO("collections 'ice/container/array.hxx' (POD)", "[collection][array][pod]")
{
    static constexpr ice::i32 test_value_1 = 0x12021;
    static constexpr ice::i32 test_value_2 = 0x23032;

    ice::HostAllocator alloc;
    ice::Array<ice::i32, ice::ContainerLogic::Trivial> objects{ alloc };

    GIVEN("an empty 'plain-old-data' Array")
    {
        REQUIRE(objects.is_empty());
        REQUIRE(objects.capacity() == 0);

        WHEN("one element is pushed")
        {
            objects.push_back(test_value_1);

            CHECK(objects.size() == 1);
            CHECK(objects.not_empty() == true);
            CHECK(objects.first() == test_value_1);
            CHECK(objects.last() == test_value_1);

            THEN("one element is poped")
            {
                objects.pop_back();

                CHECK(objects.size() == 0);
                CHECK(objects.not_empty() == false);
                CHECK(objects.is_empty() == true);

                THEN("array is shrunk")
                {
                    objects.shrink();

                    REQUIRE(objects.size() == 0);
                    REQUIRE(objects.capacity() == 0);
                }
            }

            THEN("10 elements are poped")
            {
                objects.pop_back(10);

                CHECK(objects.size() == 0);
                CHECK(objects.not_empty() == false);
                CHECK(objects.is_empty() == true);
            }
        }

        WHEN("100 elements are pushed")
        {
            for (ice::i32 i = 0; i < 100; ++i)
            {
                objects.push_back(test_value_2 + i);
            }

            CHECK(objects.size() == 100);
            CHECK(objects.capacity() >= 100);
            CHECK(objects.not_empty() == true);
            CHECK(objects.is_empty() == false);

            CHECK(objects.first() == test_value_2);
            CHECK(objects.last() == test_value_2 + 99);

            THEN("50 elements are poped")
            {
                objects.pop_back(50);

                CHECK(objects.not_empty() == true);
                CHECK(objects.is_empty() == false);
                REQUIRE(objects.size() == 50);

                THEN("array is shrunk")
                {
                    objects.shrink();

                    CHECK(objects.not_empty() == true);
                    CHECK(objects.is_empty() == false);
                    REQUIRE(objects.size() == 50);
                    REQUIRE(objects.capacity() == 50);
                }
            }

            THEN("array is cleared")
            {
                ice::ncount const saved_capacity = objects.capacity();

                objects.clear();

                CHECK(objects.not_empty() == false);
                CHECK(objects.is_empty() == true);
                REQUIRE(objects.size() == 0);
                REQUIRE(objects.capacity() == saved_capacity);
            }

            THEN("we can iterate over them")
            {
                ice::u32 elements_seen = 0;
                for ([[maybe_unused]] ice::i32 const _ : objects)
                {
                    elements_seen += 1;
                }

                CHECK(elements_seen == objects.size());
            }

            THEN("we can iterate over a span")
            {
                ice::u32 elements_seen = 0;
                for ([[maybe_unused]] ice::i32 const element : ice::Span<ice::i32>{ objects })
                {
                    elements_seen += 1;
                }

                CHECK(elements_seen == objects.size());
            }

            THEN("we can iterate over a const span")
            {
                ice::u32 elements_seen = 0;
                for ([[maybe_unused]] ice::i32 const element : ice::Span<ice::i32 const>{ objects })
                {
                    elements_seen += 1;
                }

                CHECK(elements_seen == objects.size());
            }

            THEN("we can move them")
            {
                ice::Array<ice::i32> moved_array = ice::move(objects);

                CHECK(moved_array.size() == 100);

                THEN("we can add new items")
                {
                    objects.push_back(100);

                    CHECK(objects.size() == 1);
                    REQUIRE(objects[0] == 100);
                }
            }

            THEN("we can move to an null allocator array")
            {
                ice::NullAllocator null_alloc{ };
                ice::Array<ice::i32> moved_array{ null_alloc };

                moved_array = ice::move(objects);

                CHECK(moved_array.size() == 100);

                THEN("we can add new items")
                {
                    objects.push_back(100);

                    CHECK(objects.size() == 1);
                    REQUIRE(objects[0] == 100);

                    THEN("we can move again")
                    {
                        moved_array = ice::move(objects);
                    }
                }
            }
        }
    }
}
