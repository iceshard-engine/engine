#include <catch2/catch.hpp>
#include <core/stack_string.hxx>
#include <core/memory.hxx>

SCENARIO("core :: stack_string")
{
    GIVEN("A an empty String value")
    {
        static constexpr auto stack_string_capacity = 64u;

        core::StackString<stack_string_capacity> test_string;

        // Reserve some capacity for tests
        core::string::reserve(test_string, 10);

        CHECK(core::string::size(test_string) == 0);
        CHECK(core::string::length(test_string) == 0);
        CHECK(core::string::capacity(test_string) == stack_string_capacity);
        CHECK(core::string::empty(test_string) == true);

        THEN("Assigning a value")
        {
            static constexpr const char test_string_value[] { "test_string" };
            test_string = test_string_value;

            CHECK(core::string::size(test_string) == sizeof(test_string_value));
            CHECK(core::string::length(test_string) == sizeof(test_string_value) - 1);
            CHECK(core::string::empty(test_string) == false);

            WHEN("Clearing the string")
            {
                auto saved_capacity = core::string::capacity(test_string);

                core::string::clear(test_string);

                CHECK(core::string::size(test_string) == 0);
                CHECK(core::string::length(test_string) == 0);
                CHECK(core::string::capacity(test_string) == saved_capacity);
                CHECK(core::string::empty(test_string) == true);

                //REQUIRE(test_string == "");
            }

            WHEN("Resizing the string")
            {
                auto saved_capacity = core::string::capacity(test_string);

                GIVEN("The value is zero")
                {
                    core::string::resize(test_string, 0);

                    CHECK(core::string::size(test_string) == 0);
                    CHECK(core::string::length(test_string) == 0);
                    CHECK(core::string::capacity(test_string) == saved_capacity);
                    CHECK(core::string::empty(test_string) == true);

                    //REQUIRE(test_string == "");
                }

                GIVEN("A smaller value")
                {
                    core::string::resize(test_string, 4);

                    CHECK(core::string::size(test_string) == 5);
                    CHECK(core::string::length(test_string) == 4);
                    CHECK(core::string::capacity(test_string) == saved_capacity);
                    CHECK(core::string::empty(test_string) == false);

                    //REQUIRE(test_string == "");
                }

                GIVEN("A larger value")
                {
                    core::string::resize(test_string, 20);

                    CHECK(core::string::size(test_string) == 21);
                    CHECK(core::string::length(test_string) == 20);
                    CHECK(core::string::capacity(test_string) == saved_capacity);
                    CHECK(core::string::empty(test_string) == false);

                    //REQUIRE(test_string == "");
                }
            }

            WHEN("Growing the string")
            {
                auto saved_capacity = core::string::capacity(test_string);

                GIVEN("No minimal capacity")
                {
                    core::string::grow(test_string);

                    CHECK(core::string::size(test_string) == 12);
                    CHECK(core::string::length(test_string) == 11);
                    CHECK(core::string::capacity(test_string) == saved_capacity);
                    CHECK(core::string::empty(test_string) == false);

                    //REQUIRE(test_string == "");
                }

                GIVEN("A minimal capacity")
                {
                    core::string::grow(test_string, 20);

                    CHECK(core::string::size(test_string) == 12);
                    CHECK(core::string::length(test_string) == 11);
                    CHECK(core::string::capacity(test_string) == stack_string_capacity);
                    CHECK(core::string::empty(test_string) == false);

                    //REQUIRE(test_string == "");
                }
            }

            WHEN("Setting the capacity")
            {
                GIVEN("The value is zero")
                {
                    core::string::set_capacity(test_string, 0);

                    CHECK(core::string::size(test_string) == 0);
                    CHECK(core::string::length(test_string) == 0);
                    CHECK(core::string::capacity(test_string) == stack_string_capacity);
                    CHECK(core::string::empty(test_string) == true);

                    //REQUIRE(test_string == "");
                }

                GIVEN("A smaller value")
                {
                    core::string::set_capacity(test_string, 2);

                    CHECK(core::string::size(test_string) == 2);
                    CHECK(core::string::length(test_string) == 1);
                    CHECK(core::string::capacity(test_string) == stack_string_capacity);
                    CHECK(core::string::empty(test_string) == false);

                    //REQUIRE(test_string == "");
                }

                GIVEN("A larger value")
                {
                    core::string::set_capacity(test_string, 20);

                    CHECK(core::string::size(test_string) == 12);
                    CHECK(core::string::length(test_string) == 11);
                    CHECK(core::string::capacity(test_string) == stack_string_capacity);
                    CHECK(core::string::empty(test_string) == false);

                    //REQUIRE(test_string == "");
                }
            }

            WHEN("Reserving memory")
            {
                auto saved_capacity = core::string::capacity(test_string);

                GIVEN("The value is zero")
                {
                    core::string::reserve(test_string, 0);

                    CHECK(core::string::size(test_string) == 12);
                    CHECK(core::string::length(test_string) == 11);
                    CHECK(core::string::capacity(test_string) == saved_capacity);
                    CHECK(core::string::empty(test_string) == false);

                    //REQUIRE(test_string == "");
                }

                GIVEN("A smaller value")
                {
                    core::string::reserve(test_string, 2);

                    CHECK(core::string::size(test_string) == 12);
                    CHECK(core::string::length(test_string) == 11);
                    CHECK(core::string::capacity(test_string) == saved_capacity);
                    CHECK(core::string::empty(test_string) == false);

                    //REQUIRE(test_string == "");
                }

                GIVEN("A larger value")
                {
                    core::string::reserve(test_string, 20);

                    CHECK(core::string::size(test_string) == 12);
                    CHECK(core::string::length(test_string) == 11);
                    CHECK(core::string::capacity(test_string) >= stack_string_capacity);
                    CHECK(core::string::empty(test_string) == false);

                    //REQUIRE(test_string == "");
                }
            }

            WHEN("Triming string memory")
            {
                core::string::trim(test_string);

                CHECK(core::string::size(test_string) == 12);
                CHECK(core::string::length(test_string) == 11);
                CHECK(core::string::capacity(test_string) == stack_string_capacity);
                CHECK(core::string::empty(test_string) == false);

                //REQUIRE(test_string == "");
            }
        }

        THEN("Modyfing the string")
        {
            auto saved_capacity = core::string::capacity(test_string);

            core::string::push_back(test_string, 'a');

            WHEN("Appending a character")
            {
                CHECK(core::string::size(test_string) == 2);
                CHECK(core::string::length(test_string) == 1);
                CHECK(core::string::capacity(test_string) == saved_capacity);
                CHECK(core::string::empty(test_string) == false);
            }

            core::string::push_back(test_string, "string");

            THEN("Appending a string")
            {
                CHECK(core::string::size(test_string) == 8);
                CHECK(core::string::length(test_string) == 7);
                CHECK(core::string::capacity(test_string) == saved_capacity);
                CHECK(core::string::empty(test_string) == false);
            }

            core::string::resize(test_string, 20);

            // Fill the new string with space characters.
            memset(core::string::begin(test_string), ' ', core::string::size(test_string));

            core::string::push_back(test_string, test_string);

            THEN("Resizing the string and appending itself")
            {
                CHECK(core::string::size(test_string) == 42);
                CHECK(core::string::length(test_string) == 41);
                CHECK(core::string::capacity(test_string) == stack_string_capacity);
                CHECK(core::string::empty(test_string) == false);
            }

            core::string::pop_back(test_string);

            THEN("Poping a single character")
            {
                CHECK(core::string::size(test_string) == 41);
                CHECK(core::string::length(test_string) == 40);
                CHECK(core::string::capacity(test_string) == stack_string_capacity);
                CHECK(core::string::empty(test_string) == false);
            }

            core::string::pop_back(test_string, 36);

            THEN("Poping 190 characters")
            {
                CHECK(core::string::size(test_string) == 5);
                CHECK(core::string::length(test_string) == 4);
                CHECK(core::string::capacity(test_string) >= stack_string_capacity);
                CHECK(core::string::empty(test_string) == false);
            }
        }
    }
}