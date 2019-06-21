#include <catch2/catch.hpp>
#include <core/memory.hxx>
#include <core/pod/array.hxx>

SCENARIO("core :: array")
{
    auto& alloc = core::memory::globals::default_allocator();
    auto test_array = core::pod::Array<int>{ alloc };

    GIVEN("An empty array")
    {
        WHEN("Element pushed")
        {
            core::pod::array::push_back(test_array, 0xd00b);

            CHECK(core::pod::array::size(test_array) == 1);
            CHECK(core::pod::array::any(test_array) == true);

            THEN("Element poped")
            {
                CHECK(core::pod::array::empty(test_array) == false);

                core::pod::array::pop_back(test_array);

                REQUIRE(core::pod::array::size(test_array) == 0);
                REQUIRE(core::pod::array::any(test_array) == false);
                REQUIRE(core::pod::array::empty(test_array) == true);
            }
        }

        WHEN("100 elements pushed")
        {
            CHECK(core::pod::array::empty(test_array) == true);

            for (int i = 0; i < 100; ++i)
            {
                core::pod::array::push_back(test_array, 0xd00b);
            }

            CHECK(core::pod::array::size(test_array) == 100);
            CHECK(core::pod::array::any(test_array) == true);
            CHECK(core::pod::array::empty(test_array) == false);

            THEN("50 elements poped")
            {
                for (int i = 0; i < 50; ++i)
                {
                    core::pod::array::pop_back(test_array);
                }

                CHECK(core::pod::array::any(test_array) == true);
                CHECK(core::pod::array::empty(test_array) == false);
                REQUIRE(core::pod::array::size(test_array) == 50);
            }

            THEN("Array cleared")
            {
                core::pod::array::clear(test_array);

                CHECK(core::pod::array::any(test_array) == false);
                CHECK(core::pod::array::empty(test_array) == true);
                REQUIRE(core::pod::array::size(test_array) == 0);
            }
        }
    }
}
