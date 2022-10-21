#include <catch2/catch.hpp>
#include <ice/resource_meta.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/mem_allocator_host.hxx>

namespace Catch
{
    template<>
    struct StringMaker<ice::String>
    {
        static std::string convert(ice::String const& value)
        {
            return StringMaker<std::string_view>::convert({ value._data, value._size });
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
        constexpr ice::Data test_document_data =  ice::string::data_view(test_empty_document);

        THEN("we can deserialize it into empty metadata")
        {
            ice::MutableMetadata meta{ alloc };
            ice::meta_deserialize(test_document_data, meta);

            REQUIRE(ice::hashmap::empty(meta._meta_entries));
        }
    }

    GIVEN("a simple json document")
    {
        constexpr ice::String test_empty_document = R"__({
            "name": "foo",
            "details": {
                "numbers": [1, 2, 3],
                "height": 3.14,
                "stringz": ["The", "quick", "brown", "fox"]
            }
        })__";

        constexpr ice::Data test_document_data = ice::string::data_view(test_empty_document);

        THEN("we can deserialize it into metadata")
        {
            ice::MutableMetadata meta{ alloc };
            ice::meta_deserialize(test_document_data, meta);

            CHECK(ice::hashmap::empty(meta._meta_entries) == false);

            ice::String meta_name;
            CHECK(ice::meta_read_string(meta, "name"_sid, meta_name));
            CHECK(meta_name == "foo");

            ice::f32 meta_height;
            CHECK(ice::meta_read_float(meta, "details.height"_sid, meta_height));
            CHECK(std::abs(meta_height - 3.14) < 0.00001);

            ice::Array<ice::i32> meta_numbers{ alloc };
            CHECK(ice::meta_read_int32_array(meta, "details.numbers"_sid, meta_numbers));
            CHECK(ice::array::count(meta_numbers) == 3);
            CHECK(meta_numbers[0] == 1);
            CHECK(meta_numbers[1] == 2);
            CHECK(meta_numbers[2] == 3);

            ice::Array<ice::String> meta_stringz{ alloc };
            CHECK(ice::meta_read_string_array(meta, "details.stringz"_sid, meta_stringz));
            CHECK(ice::array::count(meta_stringz) == 4);
            CHECK(meta_stringz[0] == "The");
            CHECK(meta_stringz[2] == "brown");

            THEN("we can store it in a buffer")
            {
                ice::Memory mem_meta;
                ice::meta_save(meta, alloc, mem_meta);

                THEN("we can load it again")
                {
                    ice::Metadata const_meta = ice::meta_load(ice::data_view(mem_meta));

                    ice::String meta_name_2;
                    CHECK(ice::meta_read_string(const_meta, "name"_sid, meta_name_2));
                    CHECK(meta_name_2 == "foo");

                    ice::f32 meta_height_2;
                    CHECK(ice::meta_read_float(const_meta, "details.height"_sid, meta_height_2));
                    CHECK(std::abs(meta_height_2 - 3.14) < 0.00001);

                    ice::Array<ice::i32> meta_numbers_2{ alloc };
                    CHECK(ice::meta_read_int32_array(const_meta, "details.numbers"_sid, meta_numbers_2));
                    CHECK(ice::array::count(meta_numbers_2) == 3);
                    CHECK(meta_numbers_2[0] == 1);
                    CHECK(meta_numbers_2[1] == 2);
                    CHECK(meta_numbers_2[2] == 3);

                    ice::Array<ice::String> meta_stringz_2{ alloc };
                    CHECK(ice::meta_read_string_array(const_meta, "details.stringz"_sid, meta_stringz_2));
                    CHECK(ice::array::count(meta_stringz_2) == 4);
                    CHECK(meta_stringz_2[0] == "The");
                    CHECK(meta_stringz_2[2] == "brown");
                }

                alloc.deallocate(mem_meta);
            }
        }
    }

    GIVEN("a mutable metadata object")
    {
        ice::MutableMetadata meta{ alloc };

        THEN("we can set simple values")
        {
            ice::meta_set_bool(meta, "value.bool"_sid, true);
            ice::meta_set_int32(meta, "value.int32"_sid, 123321);
            ice::meta_set_float(meta, "value.float"_sid, -2.5f);
            ice::meta_set_string(meta, "value.string"_sid, "test");

            bool meta_bool;
            ice::i32 meta_int;
            ice::f32 meta_float;
            ice::String meta_string;

            CHECK(ice::meta_read_bool(meta, "value.bool"_sid, meta_bool));
            CHECK(meta_bool == true);
            CHECK(ice::meta_read_int32(meta, "value.int32"_sid, meta_int));
            CHECK(meta_int == 123321);
            CHECK(ice::meta_read_float(meta, "value.float"_sid, meta_float));
            CHECK(meta_float == -2.5f);
            CHECK(ice::meta_read_string(meta, "value.string"_sid, meta_string));
            CHECK(meta_string == "test");
        }

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

        WHEN("it is empty")
        {
            THEN("reading simple values fails")
            {
                bool meta_bool = false;
                ice::i32 meta_int = 123;
                ice::f32 meta_float = 0.3366f;
                ice::String meta_string = "this_should_not_change!";

                CHECK(ice::meta_read_bool(meta, "value.bool"_sid, meta_bool) == false);
                CHECK(ice::meta_read_int32(meta, "value.int32"_sid, meta_int) == false);
                CHECK(ice::meta_read_float(meta, "value.float"_sid, meta_float) == false);
                CHECK(ice::meta_read_string(meta, "value.string"_sid, meta_string) == false);

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

                CHECK(ice::meta_read_bool_array(meta, "value.bool"_sid, meta_bool_arr) == false);
                CHECK(ice::meta_read_int32_array(meta, "value.int32"_sid, meta_int32_arr) == false);
                CHECK(ice::meta_read_float_array(meta, "value.float"_sid, meta_float_arr) == false);
                CHECK(ice::meta_read_string_array(meta, "value.string"_sid, meta_string_arr) == false);

                CHECK(ice::array::count(meta_bool_arr) == 0);
                CHECK(ice::array::count(meta_int32_arr) == 0);
                CHECK(ice::array::count(meta_float_arr) == 0);
                CHECK(ice::array::count(meta_string_arr) == 0);
            }
        }
    }
}
