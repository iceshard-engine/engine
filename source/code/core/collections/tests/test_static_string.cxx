/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <catch2/catch_test_macros.hpp>
#include <ice/static_string.hxx>

SCENARIO("ice :: StackString")
{
    static constexpr ice::ncount test_stack_string_capacity = 64u;
    static constexpr ice::String test_string_value{ "test_string" };

    GIVEN("A an empty String value")
    {

        ice::StaticString<test_stack_string_capacity> test_string;

        // Reserve some capacity for tests
        //ice::string::reserve(test_string, 10); = deleted

        CHECK(test_string.size() == 0);
        CHECK(test_string.capacity() == test_stack_string_capacity);
        CHECK(test_string.is_empty());

        THEN("Assigning a value")
        {
            test_string = test_string_value;

            ice::ncount const saved_size = test_string.size();
            ice::ncount const saved_capacity = test_string.capacity();

            CHECK(test_string.size() == test_string_value.size());
            CHECK(test_string.not_empty());

            WHEN("Clearing the string")
            {
                test_string.clear();

                CHECK(test_string.size() == 0);
                CHECK(test_string.capacity() == saved_capacity);
                CHECK(test_string.is_empty());

                REQUIRE(test_string == "");
            }

            WHEN("Resizing the string")
            {
                ice::ncount const saved_capacity_2 = test_string.capacity();

                GIVEN("The value is zero")
                {
                    test_string.resize(0);

                    CHECK(test_string.size() == 0);
                    CHECK(test_string.capacity() == saved_capacity_2);
                    CHECK(test_string.is_empty());

                    REQUIRE(test_string == "");
                }

                GIVEN("A smaller value")
                {
                    test_string.resize(4);

                    CHECK(test_string.size() == 4);
                    CHECK(test_string.capacity() == saved_capacity_2);
                    CHECK(test_string.not_empty());

                    REQUIRE(test_string == "test");
                }

                GIVEN("A larger value")
                {
                    test_string.resize(20);

                    CHECK(test_string.size() == 20);
                    CHECK(test_string.capacity() == saved_capacity_2);
                    CHECK(test_string.not_empty());

                    REQUIRE(test_string.substr(0, saved_size) == test_string_value);
                }
            }
        }

        THEN("Modyfing the string")
        {
            ice::ncount const saved_capacity = test_string.capacity();
            test_string.push_back('a');

            WHEN("Appending a character")
            {
                CHECK(test_string.size() == 1);
                CHECK(test_string.capacity() == saved_capacity);
                CHECK(test_string.not_empty());
            }

            test_string.push_back("string");

            THEN("Appending a string")
            {
                CHECK(test_string.size() == 7);
                CHECK(test_string.capacity() == saved_capacity);
                CHECK(test_string.not_empty());
            }

            test_string.resize(20);

            // Fill the new string with space characters.
            memset(test_string.begin(), ' ', test_string.size());

            test_string.push_back(test_string);

            THEN("Resizing the string and appending itself")
            {
                CHECK(test_string.size() == 40);
                CHECK(test_string.capacity() == test_stack_string_capacity);
                CHECK(test_string.not_empty());
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
