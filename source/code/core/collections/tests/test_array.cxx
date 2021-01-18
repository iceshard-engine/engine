#include <catch2/catch.hpp>
#include <ice/memory/memory_globals.hxx>
#include <ice/pod/array.hxx>

SCENARIO("ice :: pod :: Array")
{
    static constexpr int32_t test_value_1 = 0x12021;
    static constexpr int32_t test_value_2 = 0x23032;

    auto& alloc = ice::memory::default_allocator();
    auto test_array = ice::pod::Array<int32_t>{ alloc };

    GIVEN("an empty array")
    {
        REQUIRE(ice::pod::array::empty(test_array));
        REQUIRE(ice::pod::array::capacity(test_array) == 0);

        WHEN("one element is pushed")
        {
            ice::pod::array::push_back(test_array, test_value_1);

            CHECK(ice::pod::array::size(test_array) == 1);
            CHECK(ice::pod::array::any(test_array) == true);
            CHECK(ice::pod::array::front(test_array) == test_value_1);
            CHECK(ice::pod::array::back(test_array) == test_value_1);

            THEN("one element is poped")
            {
                ice::pod::array::pop_back(test_array);

                CHECK(ice::pod::array::size(test_array) == 0);
                CHECK(ice::pod::array::any(test_array) == false);
                CHECK(ice::pod::array::empty(test_array) == true);

                THEN("array is shrunk")
                {
                    ice::pod::array::shrink(test_array);

                    REQUIRE(ice::pod::array::size(test_array) == 0);
                    REQUIRE(ice::pod::array::capacity(test_array) == 0);
                }
            }

            THEN("10 elements are poped")
            {
                ice::pod::array::pop_back(test_array, 10);

                CHECK(ice::pod::array::size(test_array) == 0);
                CHECK(ice::pod::array::any(test_array) == false);
                CHECK(ice::pod::array::empty(test_array) == true);
            }
        }

        WHEN("100 elements are pushed")
        {
            for (int32_t i = 0; i < 100; ++i)
            {
                ice::pod::array::push_back(test_array, test_value_2 + i);
            }

            CHECK(ice::pod::array::size(test_array) == 100);
            CHECK(ice::pod::array::capacity(test_array) >= 100);
            CHECK(ice::pod::array::any(test_array) == true);
            CHECK(ice::pod::array::empty(test_array) == false);

            CHECK(ice::pod::array::front(test_array) == test_value_2);
            CHECK(ice::pod::array::back(test_array) == test_value_2 + 99);

            THEN("50 elements are poped")
            {
                ice::pod::array::pop_back(test_array, 50);

                CHECK(ice::pod::array::any(test_array) == true);
                CHECK(ice::pod::array::empty(test_array) == false);
                REQUIRE(ice::pod::array::size(test_array) == 50);

                THEN("array is shrunk")
                {
                    ice::pod::array::shrink(test_array);

                    CHECK(ice::pod::array::any(test_array) == true);
                    CHECK(ice::pod::array::empty(test_array) == false);
                    REQUIRE(ice::pod::array::size(test_array) == 50);
                    REQUIRE(ice::pod::array::capacity(test_array) == 50);
                }
            }

            THEN("array is cleared")
            {
                uint32_t const saved_capacity = ice::pod::array::capacity(test_array);

                ice::pod::array::clear(test_array);

                CHECK(ice::pod::array::any(test_array) == false);
                CHECK(ice::pod::array::empty(test_array) == true);
                REQUIRE(ice::pod::array::size(test_array) == 0);
                REQUIRE(ice::pod::array::capacity(test_array) == saved_capacity);
            }

            THEN("we can iterate over them")
            {
                uint32_t elements_seen = 0;
                for (int32_t const _ : test_array)
                {
                    elements_seen += 1;
                }

                CHECK(elements_seen == ice::pod::array::size(test_array));
            }

            THEN("we can iterate over a span")
            {
                uint32_t elements_seen = 0;
                for (uint32_t const element : ice::Span<int32_t>{ test_array })
                {
                    elements_seen += 1;
                }

                CHECK(elements_seen == ice::pod::array::size(test_array));
            }

            THEN("we can iterate over a const span")
            {
                uint32_t elements_seen = 0;
                for (uint32_t const element : ice::Span<int32_t const>{ test_array })
                {
                    elements_seen += 1;
                }

                CHECK(elements_seen == ice::pod::array::size(test_array));
            }

            THEN("we can move them")
            {
                ice::pod::Array<int32_t> moved_array = ice::move(test_array);

                CHECK(ice::pod::array::size(moved_array) == 100);

                THEN("we can add new items")
                {
                    ice::pod::array::push_back(test_array, 100);

                    CHECK(ice::pod::array::size(test_array) == 1);
                    REQUIRE(test_array[0] == 100);
                }
            }

            THEN("we can move to an null allocator array")
            {
                ice::pod::Array<int32_t> moved_array{ ice::memory::null_allocator() };

                moved_array = ice::move(test_array);

                CHECK(ice::pod::array::size(moved_array) == 100);

                THEN("we can add new items")
                {
                    moved_array = ice::move(test_array);

                    ice::pod::array::push_back(test_array, 100);

                    CHECK(ice::pod::array::size(test_array) == 1);
                    REQUIRE(test_array[0] == 100);
                }
            }
        }
    }
}
