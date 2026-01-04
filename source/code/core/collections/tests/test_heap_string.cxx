/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <catch2/catch_test_macros.hpp>
#include <ice/string.hxx>
#include <ice/heap_string.hxx>
#include <ice/mem_allocator_host.hxx>

constexpr char const test[] = "Dandielo";

SCENARIO("ice :: HeapString")
{
    static constexpr ice::String test_string_value{ "test_string" };

    ice::HostAllocator alloc{};

    GIVEN("A an empty String value")
    {
        ice::HeapString test_string{ alloc };

        // Reserve some capacity for tests
        test_string.reserve(10);

        CHECK(test_string.size() == 0);
        CHECK(test_string.capacity() == 10);
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
                GIVEN("The value is zero")
                {
                    test_string.resize(0);

                    CHECK(test_string.size() == 0);
                    CHECK(test_string.capacity() == saved_capacity);
                    CHECK(test_string.is_empty());

                    REQUIRE(test_string == "");
                }

                GIVEN("A smaller value")
                {
                    test_string.resize(4);

                    CHECK(test_string.size() == 4);
                    CHECK(test_string.capacity() == saved_capacity);
                    CHECK(test_string.not_empty());

                    REQUIRE(test_string == "test");
                }

                GIVEN("A larger value")
                {
                    test_string.resize(100);

                    CHECK(test_string.size() == 100);
                    CHECK(test_string.capacity() > saved_capacity);
                    CHECK(test_string.not_empty());

                    REQUIRE(test_string.substr(0, saved_size) == test_string_value);
                }
            }

            WHEN("Growing the string")
            {
                GIVEN("No minimal capacity")
                {
                    test_string.grow();

                    CHECK(test_string.size() == saved_size);
                    CHECK(test_string.capacity() > saved_capacity);
                    CHECK(test_string.not_empty());

                    REQUIRE(test_string == test_string_value);
                }

                GIVEN("A minimal capacity")
                {
                    test_string.grow(100);

                    CHECK(test_string.size() == saved_size);
                    CHECK(test_string.capacity() >= 100);
                    CHECK(test_string.not_empty());

                    REQUIRE(test_string == test_string_value);
                }
            }

            WHEN("Setting the capacity")
            {
                GIVEN("The value is zero")
                {
                    test_string.set_capacity(0);

                    CHECK(test_string.size() == 0);
                    CHECK(test_string.capacity() == 0);
                    CHECK(test_string.is_empty());

                    REQUIRE(test_string == "");
                }

                GIVEN("A smaller value")
                {
                    test_string.set_capacity(2);

                    CHECK(test_string.size() == 1);
                    CHECK(test_string.capacity() == 2);
                    CHECK(test_string.not_empty());

                    test_string.resize(1);
                    REQUIRE(test_string == "t");
                }

                GIVEN("A larger value")
                {
                    test_string.set_capacity(100);

                    CHECK(test_string.size() == saved_size);
                    CHECK(test_string.capacity() == 100);
                    CHECK(test_string.not_empty());

                    REQUIRE(test_string == test_string_value);
                }
            }

            WHEN("Reserving memory")
            {
                GIVEN("The value is zero")
                {
                    test_string.reserve(0);

                    CHECK(test_string.size() == saved_size);
                    CHECK(test_string.capacity() == saved_capacity);
                    CHECK(test_string.not_empty());

                    REQUIRE(test_string == test_string_value);
                }

                GIVEN("A smaller value")
                {
                    test_string.reserve(2);

                    CHECK(test_string.size() == saved_size);
                    CHECK(test_string.capacity() == saved_capacity);
                    CHECK(test_string.not_empty());

                    REQUIRE(test_string == test_string_value);
                }

                GIVEN("A larger value")
                {
                    test_string.reserve(100);

                    CHECK(test_string.size() == saved_size);
                    CHECK(test_string.capacity() >= 100);
                    CHECK(test_string.not_empty());

                    REQUIRE(test_string == test_string_value);
                }
            }

            WHEN("Triming string memory")
            {
                test_string.shrink();

                CHECK(test_string.size() == saved_size);
                CHECK(test_string.capacity() <= saved_capacity);
                CHECK(test_string.capacity() == 12);
                CHECK(test_string.not_empty());

                REQUIRE(test_string == test_string_value);
            }
        }

        THEN("Modyfing the string")
        {
            ice::ncount const saved_capacity = test_string.capacity();

            WHEN("Appending a character")
            {
                test_string.push_back('a');

                CHECK(test_string.size() == 1);
                CHECK(test_string.capacity() == saved_capacity);
                CHECK(test_string.not_empty());
            }

            THEN("Appending a string")
            {
                test_string.push_back("string");

                CHECK(test_string.size() == 6);
                CHECK(test_string.capacity() == saved_capacity);
                CHECK(test_string.not_empty());
            }

            test_string.resize(100);

            // Fill the new string with space characters.
            std::memset(test_string.begin(), ' ', test_string.size());

            ice::HeapString test_copy{ test_string };

            // We cannot push back ourselfs, as this will remove the old buffer before it even gets copied
            test_string.push_back(test_copy);

            THEN("Resizing the string and appending itself")
            {
                CHECK(test_string.size() == 200);
                CHECK(test_string.capacity() >= 201);
                CHECK(test_string.not_empty());
            }

            //THEN("Poping a single character")
            //{
            //    ice::string::pop_back(test_string);

            //    CHECK(ice::string::size(test_string) == 200);
            //    CHECK(ice::string::length(test_string) == 199);
            //    CHECK(ice::string::capacity(test_string) >= 201);
            //    CHECK(ice::string::empty(test_string) == false);
            //}

            //THEN("Poping 195 characters")
            //{
            //    ice::string::pop_back(test_string, 195);

            //    CHECK(ice::string::size(test_string) == 5);
            //    CHECK(ice::string::length(test_string) == 4);
            //    CHECK(ice::string::capacity(test_string) >= 201);
            //    CHECK(ice::string::empty(test_string) == false);
            //}
        }
    }

    GIVEN("a random string value")
    {
        ice::HeapString str{ alloc, test_string_value };

        THEN("we can iterate over it")
        {
            auto it = str.begin();
            auto const it_end = str.end();

            ice::ncount character_count = 0;
            while (it != it_end)
            {
                it += 1;
                character_count += 1;
            }

            CHECK(character_count == test_string_value.size());
        }

        THEN("we can reverse iterate over it")
        {
            auto it = str.rbegin();
            auto const it_end = str.rend();

            ice::ncount character_count = 0;
            while (it != it_end)
            {
                it += 1;
                character_count += 1;
            }

            CHECK(character_count == test_string_value.size());
        }
    }
}
