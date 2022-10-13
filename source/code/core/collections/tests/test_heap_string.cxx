#include <catch2/catch.hpp>
#include <ice/string/string.hxx>
#include <ice/string/heap_string.hxx>
#include <ice/mem_allocator_host.hxx>

SCENARIO("ice :: HeapString")
{
    static constexpr ice::String test_string_value{ "test_string" };

    ice::HostAllocator alloc{};

    GIVEN("A an empty String value")
    {
        ice::HeapString test_string{ alloc };

        // Reserve some capacity for tests
        ice::string::reserve(test_string, 10);

        CHECK(ice::string::size(test_string) == 0);
        CHECK(ice::string::capacity(test_string) == 10);
        CHECK(ice::string::empty(test_string) == true);

        THEN("Assigning a value")
        {
            test_string = test_string_value;

            ice::ucount const saved_size = ice::string::size(test_string);
            ice::ucount const saved_capacity = ice::string::capacity(test_string);

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
                GIVEN("The value is zero")
                {
                    ice::string::resize(test_string, 0);

                    CHECK(ice::string::size(test_string) == 0);
                    CHECK(ice::string::capacity(test_string) == saved_capacity);
                    CHECK(ice::string::empty(test_string) == true);

                    REQUIRE(test_string == "");
                }

                GIVEN("A smaller value")
                {
                    ice::string::resize(test_string, 4);

                    CHECK(ice::string::size(test_string) == 4);
                    CHECK(ice::string::capacity(test_string) == saved_capacity);
                    CHECK(ice::string::empty(test_string) == false);

                    REQUIRE(test_string == "test");
                }

                GIVEN("A larger value")
                {
                    ice::string::resize(test_string, 100);

                    CHECK(ice::string::size(test_string) == 100);
                    CHECK(ice::string::capacity(test_string) > saved_capacity);
                    CHECK(ice::string::empty(test_string) == false);

                    REQUIRE(ice::string::substr(test_string, 0, saved_size) == test_string_value);
                }
            }

            WHEN("Growing the string")
            {
                GIVEN("No minimal capacity")
                {
                    ice::string::grow(test_string);

                    CHECK(ice::string::size(test_string) == saved_size);
                    CHECK(ice::string::capacity(test_string) > saved_capacity);
                    CHECK(ice::string::empty(test_string) == false);

                    REQUIRE(test_string == test_string_value);
                }

                GIVEN("A minimal capacity")
                {
                    ice::string::grow(test_string, 100);

                    CHECK(ice::string::size(test_string) == saved_size);
                    CHECK(ice::string::capacity(test_string) >= 100);
                    CHECK(ice::string::empty(test_string) == false);

                    REQUIRE(test_string == test_string_value);
                }
            }

            WHEN("Setting the capacity")
            {
                GIVEN("The value is zero")
                {
                    ice::string::set_capacity(test_string, 0);

                    CHECK(ice::string::size(test_string) == 0);
                    CHECK(ice::string::capacity(test_string) == 0);
                    CHECK(ice::string::empty(test_string) == true);

                    REQUIRE(test_string == "");
                }

                GIVEN("A smaller value")
                {
                    ice::string::set_capacity(test_string, 2);

                    CHECK(ice::string::size(test_string) == 1);
                    CHECK(ice::string::capacity(test_string) == 2);
                    CHECK(ice::string::empty(test_string) == false);

                    ice::string::resize(test_string, 1);
                    REQUIRE(test_string == "t");
                }

                GIVEN("A larger value")
                {
                    ice::string::set_capacity(test_string, 100);

                    CHECK(ice::string::size(test_string) == saved_size);
                    CHECK(ice::string::capacity(test_string) == 100);
                    CHECK(ice::string::empty(test_string) == false);

                    REQUIRE(test_string == test_string_value);
                }
            }

            WHEN("Reserving memory")
            {
                GIVEN("The value is zero")
                {
                    ice::string::reserve(test_string, 0);

                    CHECK(ice::string::size(test_string) == saved_size);
                    CHECK(ice::string::capacity(test_string) == saved_capacity);
                    CHECK(ice::string::empty(test_string) == false);

                    REQUIRE(test_string == test_string_value);
                }

                GIVEN("A smaller value")
                {
                    ice::string::reserve(test_string, 2);

                    CHECK(ice::string::size(test_string) == saved_size);
                    CHECK(ice::string::capacity(test_string) == saved_capacity);
                    CHECK(ice::string::empty(test_string) == false);

                    REQUIRE(test_string == test_string_value);
                }

                GIVEN("A larger value")
                {
                    ice::string::reserve(test_string, 100);

                    CHECK(ice::string::size(test_string) == saved_size);
                    CHECK(ice::string::capacity(test_string) >= 100);
                    CHECK(ice::string::empty(test_string) == false);

                    REQUIRE(test_string == test_string_value);
                }
            }

            WHEN("Triming string memory")
            {
                ice::string::shrink(test_string);

                CHECK(ice::string::size(test_string) == saved_size);
                CHECK(ice::string::capacity(test_string) <= saved_capacity);
                CHECK(ice::string::capacity(test_string) == 12);
                CHECK(ice::string::empty(test_string) == false);

                REQUIRE(test_string == test_string_value);
            }
        }

        THEN("Modyfing the string")
        {
            auto saved_capacity = ice::string::capacity(test_string);

            WHEN("Appending a character")
            {
                ice::string::push_back(test_string, 'a');

                CHECK(ice::string::size(test_string) == 1);
                CHECK(ice::string::capacity(test_string) == saved_capacity);
                CHECK(ice::string::empty(test_string) == false);
            }

            THEN("Appending a string")
            {
                ice::string::push_back(test_string, "string");

                CHECK(ice::string::size(test_string) == 6);
                CHECK(ice::string::capacity(test_string) == saved_capacity);
                CHECK(ice::string::empty(test_string) == false);
            }

            ice::string::resize(test_string, 100);

            // Fill the new string with space characters.
            std::memset(ice::string::begin(test_string), ' ', ice::string::size(test_string));

            ice::HeapString test_copy{ test_string };

            // We cannot push back ourselfs, as this will remove the old buffer before it even gets copied
            ice::string::push_back(test_string, test_copy);

            THEN("Resizing the string and appending itself")
            {
                CHECK(ice::string::size(test_string) == 200);
                CHECK(ice::string::capacity(test_string) >= 201);
                CHECK(ice::string::empty(test_string) == false);
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
            auto it = ice::string::begin(str);
            auto const it_end = ice::string::end(str);

            ice::ucount character_count = 0;
            while (it != it_end)
            {
                it += 1;
                character_count += 1;
            }

            CHECK(character_count == ice::string::size(test_string_value));
        }

        THEN("we can reverse iterate over it")
        {
            auto it = ice::string::rbegin(str);
            auto const it_end = ice::string::rend(str);

            ice::ucount character_count = 0;
            while (it != it_end)
            {
                it += 1;
                character_count += 1;
            }

            CHECK(character_count == ice::string::size(test_string_value));
        }
    }
}
