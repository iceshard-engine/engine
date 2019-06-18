#include <catch2/catch.hpp>
#include <core/string.hxx>
#include <core/memory.hxx>

SCENARIO("core :: string")
{
    auto& alloc = core::memory::globals::default_allocator();

    GIVEN("A an emptry String value")
    {
        core::String test_string{ alloc };

        CHECK(core::string::size(test_string) == 0);
        CHECK(core::string::length(test_string) == 0);
        CHECK(core::string::empty(test_string) == true);

        WHEN("Assigning a value")
        {
            static constexpr const char test_string_value[] { "test_string" };
            test_string = test_string_value;

            CHECK(core::string::size(test_string) == sizeof(test_string_value));
            CHECK(core::string::length(test_string) == sizeof(test_string_value) - 1);
        }
    }
}
