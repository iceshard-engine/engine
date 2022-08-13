#include <catch2/catch.hpp>
#include <ice/container/array.hxx>
#include <ice/mem_allocator_host.hxx>
#include <ice/mem_allocator_null.hxx>

SCENARIO("collections 'ice/container/array.hxx'", "[collection, array, complex]")
{
    struct ComplexType
    {
        ice::u32 value;

        ice::u32 ctor = 0;
        ice::u32 ctor_copy = 0;
        ice::u32 ctor_move = 0;
        ice::u32 op_copy = 0;
        ice::u32 op_move = 0;
        ice::u32* dtor = nullptr;

        ComplexType(ice::u32 value = 0) noexcept : value{ value }, ctor { 1 } { };
        ComplexType(ComplexType&& other) noexcept : value{ ice::exchange(other.value, 0) }, ctor_move{ 1 } { };
        ComplexType(ComplexType const& other) noexcept : value{ other.value }, ctor_copy{ 1 } { };
        ~ComplexType() noexcept { if (dtor) *dtor += 1; };

        auto operator=(ComplexType&& other) noexcept -> ComplexType& { value = ice::exchange(other.value, 0); op_move += 1; return *this; };
        auto operator=(ComplexType const& other) noexcept -> ComplexType& { value = other.value; op_copy += 1; return *this; };
    };

    ice::HostAllocator alloc;
    ice::Array objects = ice::Array<ComplexType, ice::CollectionLogic::Complex>{ alloc };

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
            ice::array::push_back(objects, ComplexType{ });

            REQUIRE(ice::array::capacity(objects) >= 1);
            REQUIRE(ice::array::count(objects) == 1);

            THEN("constructors are called")
            {
                ComplexType& obj = ice::array::front(objects);

                CHECK(obj.ctor_move == 1);
                CHECK((obj.ctor + obj.ctor_copy) == 0);
                CHECK((obj.op_copy + obj.op_move) == 0);

                AND_WHEN("we remove the object the destructor is called")
                {
                    ice::u32 dtor_val = 0;
                    obj.dtor = &dtor_val;
                    ice::array::pop_back(objects, 1);

                    CHECK(dtor_val == 1);
                }
            }

            AND_WHEN("resizing the array")
            {
                ComplexType& obj = ice::array::front(objects);
                ice::u32 dtor_val = 0;
                obj.dtor = &dtor_val;

                ice::array::resize(objects, 10);

                // The old object was moved to a new location (move ctor + dtor)
                CHECK(dtor_val == 1);

                CHECK(ice::array::any(objects));
                CHECK(ice::array::empty(objects) == false);
                CHECK(ice::array::capacity(objects) >= 10);
                CHECK(ice::array::count(objects) == 10);

                THEN("constructors are called")
                {
                    ComplexType& obj_front = ice::array::front(objects);
                    ComplexType& obj_back = ice::array::back(objects);

                    CHECK(obj_front.ctor == 0);
                    CHECK(obj_front.ctor_move == 1);
                    CHECK(obj_front.ctor_copy == 0);
                    CHECK(obj_front.op_copy == 0);
                    CHECK(obj_front.op_move == 0);

                    CHECK(obj_back.ctor == 1);
                    CHECK(obj_back.ctor_move == 0);
                    CHECK(obj_back.ctor_copy == 0);
                    CHECK(obj_back.op_copy == 0);
                    CHECK(obj_back.op_move == 0);
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
                    for (ComplexType const& object : objects)
                    {
                        moved_objects += object.ctor_move;
                        copied_objects += object.ctor_copy;
                    }
                    CHECK(moved_objects == 10);
                    CHECK(copied_objects == 10);

                    AND_THEN("clearning it will destroy all objects")
                    {
                        dtor_val = 0;
                        for (ComplexType& object : objects)
                        {
                            object.dtor = &dtor_val;
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
                    for (ComplexType& object : objects)
                    {
                        object.dtor = &dtor_val;
                    }

                    ice::Array moved_objects = ice::move(objects);
                    CHECK(dtor_val == 0);

                    CHECK(ice::array::capacity(objects) == 0);
                    CHECK(ice::array::empty(objects));
                    CHECK(ice::array::any(objects) == false);
                    CHECK(ice::array::count(objects) == 0);

                    for (ComplexType const& object : moved_objects)
                    {
                        CHECK((object.ctor + object.ctor_move) == 1);
                        CHECK(object.ctor_copy == 0);
                        CHECK(object.op_copy == 0);
                        CHECK(object.op_move == 0);
                    }
                }
            }
        }
    }
}

SCENARIO("collections 'ice/container/array.hxx' (POD)", "[collection, array, pod]")
{
    static constexpr ice::i32 test_value_1 = 0x12021;
    static constexpr ice::i32 test_value_2 = 0x23032;

    ice::HostAllocator alloc;
    ice::Array objects = ice::Array<ice::i32, ice::CollectionLogic::PlainOldData>{ alloc };

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
