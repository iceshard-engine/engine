#include <catch2/catch.hpp>
#include <ice/data.hxx>
#include <ice/span.hxx>

SCENARIO("ice :: Data")
{
    ice::Data data{ };

    WHEN("Empty")
    {
        CHECK(data.location == nullptr);
        CHECK(data.size == 0);
        CHECK(data.alignment == 0);
    }

    WHEN("Creating a view to a regular value")
    {
        ice::u64 const value = 0x42069;

        data = ice::data_view(value);

        CHECK(data.location == &value);
        CHECK(data.size == sizeof(ice::u64));
        CHECK(data.alignment == alignof(ice::u64));
    }

    WHEN("Creating a view to a span")
    {
        ice::u64 const values[]{
            0,
            0x32,
            0x42069,
            0,
            0x32,
            0x42069,
            0,
            0x32,
            0x42069,
        };

        ice::Span<ice::u64 const> values_span = ice::make_span(values);

        data = ice::data_view(values_span);

        CHECK(data.location == values_span.data());
        CHECK(data.size == values_span.size_bytes());
        CHECK(data.alignment == alignof(ice::u64));
    }
}
