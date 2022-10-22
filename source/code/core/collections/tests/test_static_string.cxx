/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <catch2/catch.hpp>
#include <ice/string/static_string.hxx>


SCENARIO("ice :: StackString")
{
    static constexpr ice::ucount test_stack_string_capacity = 64u;
    static constexpr ice::String test_string_value{ "test_string" };

    GIVEN("A an empty String value")
    {

        ice::StaticString<test_stack_string_capacity> test_string;

        // Reserve some capacity for tests
        //ice::string::reserve(test_string, 10); = deleted

        CHECK(ice::string::size(test_string) == 0);
        CHECK(ice::string::capacity(test_string) == test_stack_string_capacity);
        CHECK(ice::string::empty(test_string) == true);

        THEN("Assigning a value")
        {
            test_string = test_string_value;

            uint32_t const saved_size = ice::string::size(test_string);
            uint32_t const saved_capacity = ice::string::capacity(test_string);

            CHECK(ice::string::size(test_string) == ice::string::size(test_string_value));
            CHECK(ice::string::empty(test_string) == false);

            WHEN("Clearing the string")
            {

                ice::string::clear(test_string);

                CHECK(ice::string::size(test_string) == 0);
                CHECK(ice::string::capacity(test_string) == saved_capacity);
                CHECK(ice::string::empty(test_string) == true);

                REQUIRE(test_string == "");
            }

            WHEN("Resizing the string")
            {
                auto saved_capacity_2 = ice::string::capacity(test_string);

                GIVEN("The value is zero")
                {
                    ice::string::resize(test_string, 0);

                    CHECK(ice::string::size(test_string) == 0);
                    CHECK(ice::string::capacity(test_string) == saved_capacity_2);
                    CHECK(ice::string::empty(test_string) == true);

                    REQUIRE(test_string == "");
                }

                GIVEN("A smaller value")
                {
                    ice::string::resize(test_string, 4);

                    CHECK(ice::string::size(test_string) == 4);
                    CHECK(ice::string::capacity(test_string) == saved_capacity_2);
                    CHECK(ice::string::empty(test_string) == false);

                    REQUIRE(test_string == "test");
                }

                GIVEN("A larger value")
                {
                    ice::string::resize(test_string, 20);

                    CHECK(ice::string::size(test_string) == 20);
                    CHECK(ice::string::capacity(test_string) == saved_capacity_2);
                    CHECK(ice::string::empty(test_string) == false);

                    REQUIRE(ice::string::substr(test_string, 0, saved_size) == test_string_value);
                }
            }
        }

        THEN("Modyfing the string")
        {
            uint32_t const saved_capacity = ice::string::capacity(test_string);

            ice::string::push_back(test_string, 'a');

            WHEN("Appending a character")
            {
                CHECK(ice::string::size(test_string) == 1);
                CHECK(ice::string::capacity(test_string) == saved_capacity);
                CHECK(ice::string::empty(test_string) == false);
            }

            ice::string::push_back(test_string, "string");

            THEN("Appending a string")
            {
                CHECK(ice::string::size(test_string) == 7);
                CHECK(ice::string::capacity(test_string) == saved_capacity);
                CHECK(ice::string::empty(test_string) == false);
            }

            ice::string::resize(test_string, 20);

            // Fill the new string with space characters.
            memset(ice::string::begin(test_string), ' ', ice::string::size(test_string));

            ice::string::push_back(test_string, test_string);

            THEN("Resizing the string and appending itself")
            {
                CHECK(ice::string::size(test_string) == 40);
                CHECK(ice::string::capacity(test_string) == test_stack_string_capacity);
                CHECK(ice::string::empty(test_string) == false);
            }

            //ice::string::pop_back(test_string);

            //THEN("Poping a single character")
            //{
            //    CHECK(ice::string::size(test_string) == 41);
            //    CHECK(ice::string::length(test_string) == 40);
            //    CHECK(ice::string::capacity(test_string) == test_stack_string_capacity);
            //    CHECK(ice::string::empty(test_string) == false);
            //}

            //ice::string::pop_back(test_string, 36);

            //THEN("Poping 190 characters")
            //{
            //    CHECK(ice::string::size(test_string) == 5);
            //    CHECK(ice::string::length(test_string) == 4);
            //    CHECK(ice::string::capacity(test_string) >= test_stack_string_capacity);
            //    CHECK(ice::string::empty(test_string) == false);
            //}
        }
    }
}
