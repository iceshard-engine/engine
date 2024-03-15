/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <catch2/catch_test_macros.hpp>
#include <ice/container/array.hxx>
#include <ice/mem_allocator_host.hxx>
#include <ice/mem_allocator_null.hxx>
#include "util_tracking_object.hxx"

SCENARIO("collections 'ice/container/array.hxx'", "[collection][array][complex]")
{
    ice::HostAllocator alloc;
    ice::Array<Test_TrackingObject, ice::ContainerLogic::Complex> objects{ alloc };

    GIVEN("an empty Array object")
    {
        CHECK(ice::array::empty(objects));
        CHECK(ice::array::any(objects) == false);
        CHECK(ice::array::capacity(objects) == 0);
        CHECK(ice::array::count(objects) == 0);

        // We force a capacity of 1, so we ensure a reallocation in later.
        ice::array::set_capacity(objects, 1);

        WHEN("adding a new object")
        {
            ice::array::push_back(objects, Test_TrackingObject{ });

            REQUIRE(ice::array::capacity(objects) >= 1);
            REQUIRE(ice::array::count(objects) == 1);

            THEN("constructors are called")
            {
                Test_TrackingObject& obj = ice::array::front(objects);

                CHECK(obj == Test_ObjectEvents{ .test_ctor_move = 1 });

                AND_WHEN("we remove the object the destructor is called")
                {
                    ice::ucount dtor_val = 0;
                    obj.data.test_dtor = &dtor_val;
                    ice::array::pop_back(objects, 1);

                    CHECK(dtor_val == 1);
                }
            }

            AND_WHEN("resizing the array")
            {
                Test_TrackingObject& obj = ice::array::front(objects);
                ice::u32 dtor_val = 0;
                obj.data.test_dtor = &dtor_val;

                ice::array::resize(objects, 10);

                // The old object was moved to a new location (move ctor + dtor)
                CHECK(dtor_val == 1);

                CHECK(ice::array::any(objects));
                CHECK(ice::array::empty(objects) == false);
                CHECK(ice::array::capacity(objects) >= 10);
                CHECK(ice::array::count(objects) == 10);

                THEN("constructors are called")
                {
                    Test_TrackingObject& obj_front = ice::array::front(objects);
                    Test_TrackingObject& obj_back = ice::array::back(objects);

                    CHECK(obj_front == Test_ObjectEvents{ .test_ctor_move = 1 });
                    CHECK(obj_back == Test_ObjectEvents{ .test_ctor = 1 });
                }

                AND_THEN("we can copy and push back the same elements")
                {
                    ice::Array array_copy = objects;
                    ice::array::push_back(objects, array_copy);

                    CHECK(ice::array::capacity(objects) >= 20);
                    CHECK(ice::array::any(objects));
                    CHECK(ice::array::empty(objects) == false);
                    CHECK(ice::array::count(objects) == 20);

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

                        ice::array::clear(objects);

                        CHECK(dtor_val == 20);
                        CHECK(ice::array::capacity(objects) > 0);
                        CHECK(ice::array::empty(objects));
                        CHECK(ice::array::any(objects) == false);
                        CHECK(ice::array::count(objects) == 0);
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

                    CHECK(ice::array::capacity(objects) == 0);
                    CHECK(ice::array::empty(objects));
                    CHECK(ice::array::any(objects) == false);
                    CHECK(ice::array::count(objects) == 0);

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
        REQUIRE(ice::array::empty(objects));
        REQUIRE(ice::array::capacity(objects) == 0);

        WHEN("one element is pushed")
        {
            ice::array::push_back(objects, test_value_1);

            CHECK(ice::array::count(objects) == 1);
            CHECK(ice::array::any(objects) == true);
            CHECK(ice::array::front(objects) == test_value_1);
            CHECK(ice::array::back(objects) == test_value_1);

            THEN("one element is poped")
            {
                ice::array::pop_back(objects);

                CHECK(ice::array::count(objects) == 0);
                CHECK(ice::array::any(objects) == false);
                CHECK(ice::array::empty(objects) == true);

                THEN("array is shrunk")
                {
                    ice::array::shrink(objects);

                    REQUIRE(ice::array::count(objects) == 0);
                    REQUIRE(ice::array::capacity(objects) == 0);
                }
            }

            THEN("10 elements are poped")
            {
                ice::array::pop_back(objects, 10);

                CHECK(ice::array::count(objects) == 0);
                CHECK(ice::array::any(objects) == false);
                CHECK(ice::array::empty(objects) == true);
            }
        }

        WHEN("100 elements are pushed")
        {
            for (ice::i32 i = 0; i < 100; ++i)
            {
                ice::array::push_back(objects, test_value_2 + i);
            }

            CHECK(ice::array::count(objects) == 100);
            CHECK(ice::array::capacity(objects) >= 100);
            CHECK(ice::array::any(objects) == true);
            CHECK(ice::array::empty(objects) == false);

            CHECK(ice::array::front(objects) == test_value_2);
            CHECK(ice::array::back(objects) == test_value_2 + 99);

            THEN("50 elements are poped")
            {
                ice::array::pop_back(objects, 50);

                CHECK(ice::array::any(objects) == true);
                CHECK(ice::array::empty(objects) == false);
                REQUIRE(ice::array::count(objects) == 50);

                THEN("array is shrunk")
                {
                    ice::array::shrink(objects);

                    CHECK(ice::array::any(objects) == true);
                    CHECK(ice::array::empty(objects) == false);
                    REQUIRE(ice::array::count(objects) == 50);
                    REQUIRE(ice::array::capacity(objects) == 50);
                }
            }

            THEN("array is cleared")
            {
                ice::u32 const saved_capacity = ice::array::capacity(objects);

                ice::array::clear(objects);

                CHECK(ice::array::any(objects) == false);
                CHECK(ice::array::empty(objects) == true);
                REQUIRE(ice::array::count(objects) == 0);
                REQUIRE(ice::array::capacity(objects) == saved_capacity);
            }

            THEN("we can iterate over them")
            {
                ice::u32 elements_seen = 0;
                for ([[maybe_unused]] ice::i32 const _ : objects)
                {
                    elements_seen += 1;
                }

                CHECK(elements_seen == ice::array::count(objects));
            }

            THEN("we can iterate over a span")
            {
                ice::u32 elements_seen = 0;
                for ([[maybe_unused]] ice::i32 const element : ice::Span<ice::i32>{ objects })
                {
                    elements_seen += 1;
                }

                CHECK(elements_seen == ice::array::count(objects));
            }

            THEN("we can iterate over a const span")
            {
                ice::u32 elements_seen = 0;
                for ([[maybe_unused]] ice::i32 const element : ice::Span<ice::i32 const>{ objects })
                {
                    elements_seen += 1;
                }

                CHECK(elements_seen == ice::array::count(objects));
            }

            THEN("we can move them")
            {
                ice::Array<ice::i32> moved_array = ice::move(objects);

                CHECK(ice::array::count(moved_array) == 100);

                THEN("we can add new items")
                {
                    ice::array::push_back(objects, 100);

                    CHECK(ice::array::count(objects) == 1);
                    REQUIRE(objects[0] == 100);
                }
            }

            THEN("we can move to an null allocator array")
            {
                ice::NullAllocator null_alloc{ };
                ice::Array<ice::i32> moved_array{ null_alloc };

                moved_array = ice::move(objects);

                CHECK(ice::array::count(moved_array) == 100);

                THEN("we can add new items")
                {
                    ice::array::push_back(objects, 100);

                    CHECK(ice::array::count(objects) == 1);
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
