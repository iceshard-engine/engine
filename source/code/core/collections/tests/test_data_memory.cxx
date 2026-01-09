/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <catch2/catch_test_macros.hpp>
#include <ice/mem_info.hxx>
#include <ice/mem_data.hxx>
#include <ice/span.hxx>

SCENARIO("ice :: Data")
{
    using ice::operator""_B;

    ice::Data data{ };

    WHEN("Empty")
    {
        CHECK(data.location == nullptr);
        CHECK(data.size == 0_B);
        CHECK(data.alignment == ice::ualign::invalid);
    }

    WHEN("Creating a view to a regular value")
    {
        ice::u64 const value = 0x42069;

        data = ice::data_view(value);

        CHECK(data.location == &value);
        CHECK(data.size == ice::size_of<ice::u64>);
        CHECK(data.alignment == ice::align_of<ice::u64>);
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

        ice::Span<ice::u64 const> values_span = values;

        data = ice::data_view(values_span);

        CHECK(data.location == values_span.data());
        CHECK(data.size == values_span.size());
        CHECK(data.alignment == ice::align_of<ice::Span<ice::u64 const>::ValueType>);
    }
}
