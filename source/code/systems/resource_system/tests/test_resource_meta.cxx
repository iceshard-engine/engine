/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <catch2/catch_test_macros.hpp>
#include <ice/config.hxx>
#include <ice/config/config_builder.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/mem_allocator_host.hxx>

namespace Catch
{
    template<>
    struct StringMaker<ice::String>
    {
        static std::string convert(ice::String const& value)
        {
            return StringMaker<std::string_view>::convert(value);
        }
    };
}

SCENARIO("resource_system 'ice/resource_meta.hxx'", "[resource][metadata]")
{
    using ice::operator""_sid;

    ice::HostAllocator alloc{ };

    GIVEN("a empty json document")
    {
        constexpr ice::String test_empty_document = "{}";

        THEN("we can deserialize it into empty metadata")
        {
            ice::Memory configmem;
            ice::Config const cfg = ice::config::from_json(alloc, test_empty_document, configmem);

            CHECK(configmem.location == nullptr);
            CHECK(cfg._data == nullptr);
        }
    }

    GIVEN("a simple json document")
    {
        constexpr ice::String test_simple_document = R"__({
            "name": "foo",
            "details": {
                "numbers": [1, 2, 3],
                "height": 3.14,
                "strings": ["The", "quick", "brown", "fox"]
            }
        })__";

        THEN("we can deserialize it into metadata")
        {
            ice::Memory configmem;
            ice::Config const meta = ice::config::from_json(alloc, test_simple_document, configmem);

            REQUIRE(configmem.location != nullptr);
            REQUIRE(meta._data != nullptr);

            ice::String meta_name;
            meta_name == "asd";

            CHECK(ice::config::get(meta, "name", meta_name));
            CHECK(meta_name == "foo");

            ice::f32 meta_height;
            CHECK(ice::config::get(meta, "details.height", meta_height));
            CHECK(std::abs(meta_height - 3.14) < 0.00001);

            ice::Array<ice::i32> meta_numbers{ alloc };
            CHECK(ice::config::get_array(meta, "details.numbers", meta_numbers));
            CHECK(ice::array::count(meta_numbers) == 3);
            CHECK(meta_numbers[0] == 1);
            CHECK(meta_numbers[1] == 2);
            CHECK(meta_numbers[2] == 3);

            ice::Array<ice::String> meta_strings{ alloc };
            CHECK(ice::config::get_array(meta, "details.strings", meta_strings));
            CHECK(ice::array::count(meta_strings) == 4);
            CHECK(meta_strings[0] == "The");
            CHECK(meta_strings[2] == "brown");

            THEN("we can store it in a buffer")
            {
                ice::ConfigBuilder meta_builder{ alloc };

                // TODO: Implement mering configs into builder
                ice::config::from_json(meta_builder, test_simple_document);
                ice::Memory mem_meta = meta_builder.finalize(alloc);

                THEN("we can load it again")
                {
                    ice::Config const const_meta = ice::config::from_data(ice::data_view(mem_meta));

                    ice::String meta_name_2;
                    CHECK(ice::config::get(const_meta, "name", meta_name_2));
                    CHECK(meta_name_2 == "foo");

                    ice::f32 meta_height_2;
                    CHECK(ice::config::get(const_meta, "details.height", meta_height_2));
                    CHECK(std::abs(meta_height_2 - 3.14) < 0.00001);

                    ice::Array<ice::i32> meta_numbers_2{ alloc };
                    CHECK(ice::config::get_array(const_meta, "details.numbers", meta_numbers_2));
                    CHECK(ice::array::count(meta_numbers_2) == 3);
                    CHECK(meta_numbers_2[0] == 1);
                    CHECK(meta_numbers_2[1] == 2);
                    CHECK(meta_numbers_2[2] == 3);

                    ice::Array<ice::String> meta_strings_2{ alloc };
                    CHECK(ice::config::get_array(const_meta, "details.strings", meta_strings_2));
                    CHECK(ice::array::count(meta_strings_2) == 4);
                    CHECK(meta_strings_2[0] == "The");
                    CHECK(meta_strings_2[2] == "brown");
                }

                alloc.deallocate(mem_meta);
            }
            alloc.deallocate(configmem);
        }
    }

    GIVEN("a mutable metadata object")
    {
        ice::ConfigBuilder meta{ alloc };

        THEN("we can set simple values")
        {
            meta["value"]["bool"] = true;
            meta["value"]["int32"] = 123321;
            meta["value"]["float"] = -2.5f;
            meta["value"]["string"] = "test";

            bool meta_bool;
            ice::i32 meta_int;
            ice::f32 meta_float;
            ice::String meta_string;

            ice::Memory configmem = meta.finalize(alloc);
            ice::Config const cfg = ice::config::from_data(ice::data_view(configmem));

            CHECK(ice::config::get(cfg, "value.bool", meta_bool));
            CHECK(meta_bool == true);
            CHECK(ice::config::get(cfg, "value.int32", meta_int));
            CHECK(meta_int == 123321);
            CHECK(ice::config::get(cfg, "value.float", meta_float));
            CHECK(meta_float == -2.5f);
            CHECK(ice::config::get(cfg, "value.string", meta_string));
            CHECK(meta_string == "test");

            alloc.deallocate(configmem);
        }

#if 0
        THEN("we can set array values")
        {
            bool const test_bool_values[]{ true, false, true };
            ice::i32 const test_int32_values[]{ 1, 2, -3, -4, 123, -7764 };
            ice::f32 const test_float_values[]{ 1.22f, 2.1f, -3.854f, -4.3f, 123.9f, -7764.f };
            ice::String const test_string_values[]{ "this", "works", "!" };

            ice::meta_set_bool_array(meta, "value.bool"_sid, test_bool_values);
            ice::meta_set_int32_array(meta, "value.int32"_sid, test_int32_values);
            ice::meta_set_float_array(meta, "value.float"_sid, test_float_values);
            ice::meta_set_string_array(meta, "value.string"_sid, test_string_values);

            ice::Array<bool> meta_bool_arr{ alloc };
            ice::Array<ice::i32> meta_int32_arr{ alloc };
            ice::Array<ice::f32> meta_float_arr{ alloc };
            ice::Array<ice::String> meta_string_arr{ alloc };

            CHECK(ice::meta_read_bool_array(meta, "value.bool"_sid, meta_bool_arr));
            CHECK(ice::meta_read_int32_array(meta, "value.int32"_sid, meta_int32_arr));
            CHECK(ice::meta_read_float_array(meta, "value.float"_sid, meta_float_arr));
            CHECK(ice::meta_read_string_array(meta, "value.string"_sid, meta_string_arr));

            CHECK(ice::array::count(meta_bool_arr) == 3);
            CHECK(ice::array::count(meta_int32_arr) == 6);
            CHECK(ice::array::count(meta_float_arr) == 6);
            CHECK(ice::array::count(meta_string_arr) == 3);
        }
#endif

        WHEN("it is empty")
        {
            ice::Config const empty_config;

            THEN("reading simple values fails")
            {
                bool meta_bool = false;
                ice::i32 meta_int = 123;
                ice::f32 meta_float = 0.3366f;
                ice::String meta_string = "this_should_not_change!";

                CHECK(ice::config::get(empty_config, "value.bool", meta_bool) == ice::E_Fail);
                CHECK(ice::config::get(empty_config, "value.int32", meta_int) == ice::E_Fail);
                CHECK(ice::config::get(empty_config, "value.float", meta_float) == ice::E_Fail);
                CHECK(ice::config::get(empty_config, "value.string", meta_string) == ice::E_Fail);

                CHECK(meta_bool == false);
                CHECK(meta_int == 123);
                CHECK(meta_float == 0.3366f);
                CHECK(meta_string == "this_should_not_change!");
            }

            THEN("reading array values fails")
            {
                ice::Array<bool> meta_bool_arr{ alloc };
                ice::Array<ice::i32> meta_int32_arr{ alloc };
                ice::Array<ice::f32> meta_float_arr{ alloc };
                ice::Array<ice::String> meta_string_arr{ alloc };

                CHECK(ice::config::get_array(empty_config, "value.bool", meta_bool_arr) == ice::E_Fail);
                CHECK(ice::config::get_array(empty_config, "value.int32", meta_int32_arr) == ice::E_Fail);
                CHECK(ice::config::get_array(empty_config, "value.float", meta_float_arr) == ice::E_Fail);
                CHECK(ice::config::get_array(empty_config, "value.string", meta_string_arr) == ice::E_Fail);

                CHECK(ice::array::count(meta_bool_arr) == 0);
                CHECK(ice::array::count(meta_int32_arr) == 0);
                CHECK(ice::array::count(meta_float_arr) == 0);
                CHECK(ice::array::count(meta_string_arr) == 0);
            }
        }
    }
}
