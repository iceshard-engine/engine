#include <ice/resource_meta.hxx>
#include <ice/pod/hash.hxx>
#include <ice/memory/memory_globals.hxx>
#include <ice/memory/forward_allocator.hxx>
#include <ice/memory/pointer_arithmetic.hxx>
#include <ice/stringid.hxx>
#include <ice/string.hxx>
#include <ice/assert.hxx>

#include <rapidjson/document.h>
#undef assert

namespace ice
{

    namespace detail
    {

        constexpr ice::Data Constant_FileHeaderData_MetadataFile{
            .location = ice::string::data(Constant_FileHeader_MetadataFile),
            .size = ice::string::size(Constant_FileHeader_MetadataFile),
            .alignment = 1
        };

        bool get_entry(
            ice::Metadata const& meta,
            ice::StringID_Arg key,
            MetadataEntryType expected_type,
            MetadataEntry& entry_out
        ) noexcept
        {
            entry_out = ice::pod::hash::get(
                meta._meta_entries,
                ice::hash(key),
                MetadataEntry{ .data_type = MetadataEntryType::Invalid }
            );
            return entry_out.data_type == expected_type;
        }

        void deserialize_json_array_helper(
            rapidjson::GenericArray<true, rapidjson::Value> const& arr,
            ice::StringID_Arg key,
            ice::MutableMetadata& meta
        ) noexcept
        {
            ice::u32 const count = arr.Size();

            static_assert(sizeof(ice::String) == 16);
            static_assert(sizeof(ice::String) >= sizeof(MetadataEntry));
            ice::memory::ForwardAllocator alloc{ ice::memory::default_scratch_allocator(), count * 16 * 64 };

            if (arr[0].IsBool())
            {
                ice::pod::Array<bool> final_values{ alloc };
                ice::pod::array::reserve(final_values, count);

                for (auto const& value : arr)
                {
                    if (value.IsBool())
                    {
                        ice::pod::array::push_back(final_values, value.GetBool());
                    }
                }

                ice::meta_set_bool_array(meta, key, ice::Span<bool>{ final_values });
            }
            else if (arr[0].IsInt())
            {
                ice::pod::Array<ice::i32> final_values{ alloc };
                ice::pod::array::reserve(final_values, count);

                for (auto const& value : arr)
                {
                    if (value.IsInt())
                    {
                        ice::pod::array::push_back(final_values, value.GetInt());
                    }
                }

                ice::meta_set_int32_array(meta, key, ice::Span<ice::i32>{ final_values });
            }
            else if (arr[0].IsFloat())
            {
                ice::pod::Array<ice::f32> final_values{ alloc };
                ice::pod::array::reserve(final_values, count);

                for (auto const& value : arr)
                {
                    if (value.IsFloat())
                    {
                        ice::pod::array::push_back(final_values, value.GetFloat());
                    }
                }

                ice::meta_set_float_array(meta, key, ice::Span<ice::f32>{ final_values });
            }
            else if (arr[0].IsDouble())
            {
                ice::pod::Array<ice::f32> final_values{ alloc };
                ice::pod::array::reserve(final_values, count);

                for (auto const& value : arr)
                {
                    if (value.IsDouble())
                    {
                        ice::pod::array::push_back(final_values, static_cast<ice::f32>(value.GetDouble()));
                    }
                }

                ice::meta_set_float_array(meta, key, ice::Span<ice::f32>{ final_values });
            }
            else if (arr[0].IsString())
            {
                ice::pod::Array<ice::Utf8String> final_values{ alloc };
                ice::pod::array::reserve(final_values, count);

                for (auto const& value : arr)
                {
                    if (value.IsString())
                    {
                        ice::pod::array::push_back(final_values, reinterpret_cast<char8_t const*>(value.GetString()));
                    }
                }

                ice::meta_set_utf8_array(meta, key, ice::Span<ice::Utf8String>{ final_values });
            }
            else
            {
                //IS_ASSERT(false, "Unknown value type in resource meta!");
            }
        }

        void deserialize_json_meta_helper(
            rapidjson::Value const& object,
            std::string key,
            ice::MutableMetadata& meta
        ) noexcept
        {
            for (auto const& entry : object.GetObject())
            {
                if (entry.value.IsObject())
                {
                    deserialize_json_meta_helper(entry.value, key + entry.name.GetString() + '.', meta);
                }
                else if (entry.value.IsArray())
                {
                    auto values = entry.value.GetArray();
                    ice::u32 const count = values.Size();

                    if (count > 0)
                    {
                        std::string field_key = key.substr(1) + entry.name.GetString();
                        deserialize_json_array_helper(values, ice::stringid(field_key), meta);
                    }
                }
                else
                {
                    std::string field_key = key.substr(1) + entry.name.GetString();

                    if (entry.value.IsBool())
                    {
                        ice::meta_set_bool(meta, ice::stringid(field_key), entry.value.GetBool());
                    }
                    else if (entry.value.IsInt())
                    {
                        ice::meta_set_int32(meta, ice::stringid(field_key), entry.value.GetInt());
                    }
                    else if (entry.value.IsFloat())
                    {
                        ice::meta_set_float(meta, ice::stringid(field_key), entry.value.GetFloat());
                    }
                    else if (entry.value.IsDouble())
                    {
                        ice::meta_set_float(meta, ice::stringid(field_key), static_cast<ice::f32>(entry.value.GetDouble()));
                    }
                    else if (entry.value.IsString())
                    {
                        ice::meta_set_utf8(meta, ice::stringid(field_key), reinterpret_cast<char8_t const*>(entry.value.GetString()));
                    }
                    else
                    {
                        ICE_ASSERT(false, "Unknown value type in resource meta!");
                    }
                }
            }
        }

        void deserialize_json_meta(ice::Data data, ice::MutableMetadata& meta) noexcept
        {
            rapidjson::Document doc;
            doc.Parse(reinterpret_cast<char const*>(data.location), data.size);
            //IS_ASSERT(doc.IsObject(), "The resource metadata is not a valid Json object!");

            deserialize_json_meta_helper(doc.GetObject(), ".", meta);
        }

        void deserialize_binary_meta(ice::Data data, ice::MutableMetadata& meta) noexcept
        {
            char const* it = reinterpret_cast<char const*>(data.location);

            ice::u32 const hash_count = *reinterpret_cast<ice::u32 const*>(it + 0);
            ice::u32 const value_count = *reinterpret_cast<ice::u32 const*>(it + 4);
            it += sizeof(ice::u32) * 2;

            ice::u32 const hash_offset = *reinterpret_cast<ice::u32 const*>(it + 0);
            ice::u32 const value_offset = *reinterpret_cast<ice::u32 const*>(it + 4);
            ice::u32 const data_offset = *reinterpret_cast<ice::u32 const*>(it + 8);

            ice::u32 const hash_type_size = sizeof(*meta._meta_entries._hash._data);
            ice::u32 const value_type_size = sizeof(*meta._meta_entries._data._data);

            {
                void const* hash_it = ice::memory::ptr_add(data.location, hash_offset);

                if constexpr (ice::build::is_release == false)
                {
                    [[maybe_unused]]
                    void const* hash_end = ice::memory::ptr_add(hash_it, hash_count * hash_type_size);
                    ICE_ASSERT(
                        ice::memory::ptr_distance(data.location, hash_end) < static_cast<ice::i32>(data.size),
                        "Moved past the data buffer!"
                    );
                }

                ice::pod::array::resize(meta._meta_entries._hash, hash_count);
                ice::memcpy(meta._meta_entries._hash._data, hash_it, hash_count * hash_type_size);
            }

            {
                void const* value_it = ice::memory::ptr_add(data.location, value_offset);

                if constexpr (ice::build::is_release == false)
                {
                    [[maybe_unused]]
                    auto const value_end = ice::memory::ptr_add(value_it, hash_count * hash_type_size);
                    ICE_ASSERT(
                        ice::memory::ptr_distance(data.location, value_end) < static_cast<ice::i32>(data.size),
                        "Moved past the data buffer!"
                    );
                }

                ice::pod::array::resize(meta._meta_entries._data, value_count);
                ice::memcpy(meta._meta_entries._data._data, value_it, value_count * value_type_size);
            }

            {
                void const* data_it = ice::memory::ptr_add(data.location, data_offset);
                ice::u32 const remaining_size = data.size - data_offset;

                ice::buffer::reserve(meta._additional_data, remaining_size);
                ice::buffer::append(meta._additional_data, ice::data_view(data_it, remaining_size, alignof(MetadataEntry)));
            }
        }

    } // namespace detail

    bool meta_has_entry(ice::Metadata const& meta, ice::StringID_Arg key) noexcept
    {
        detail::MetadataEntry entry;
        return detail::get_entry(meta, key, detail::MetadataEntryType::Invalid, entry) == false;
    }

    auto meta_read_bool(
        ice::Metadata const& meta,
        ice::StringID_Arg key,
        bool& result
    ) noexcept -> bool
    {
        detail::MetadataEntry entry;
        bool const valid = detail::get_entry(meta, key, detail::MetadataEntryType::Boolean, entry);

        if (valid && entry.data_count == 0)
        {
            result = entry.value_int != 0;
        }

        return valid;
    }

    auto meta_read_int32(
        ice::Metadata const& meta,
        ice::StringID_Arg key,
        ice::i32& result
    ) noexcept -> bool
    {
        detail::MetadataEntry entry;
        bool const valid = detail::get_entry(meta, key, detail::MetadataEntryType::Integer, entry);

        if (valid && entry.data_count == 0)
        {
            result = entry.value_int;
        }

        return valid;
    }

    auto meta_read_float(
        ice::Metadata const& meta,
        ice::StringID_Arg key,
        ice::f32& result
    ) noexcept -> bool
    {
        detail::MetadataEntry entry;
        bool const valid = detail::get_entry(meta, key, detail::MetadataEntryType::Float, entry);

        if (valid && entry.data_count == 0)
        {
            result = entry.value_float;
        }

        return valid;
    }

    auto meta_read_string(
        ice::Metadata const& meta,
        ice::StringID_Arg key,
        ice::String& result
    ) noexcept -> bool
    {
        detail::MetadataEntry entry;
        bool const valid = detail::get_entry(meta, key, detail::MetadataEntryType::String, entry);

        if (valid && entry.data_count == 0)
        {
            char const* string_beg = reinterpret_cast<char const*>(meta._additional_data.location) + entry.value_buffer.offset;
            result = ice::String{ string_beg, entry.value_buffer.size };
        }

        return valid;
    }

    auto meta_read_utf8(
        ice::Metadata const& meta,
        ice::StringID_Arg key,
        ice::Utf8String& result
    ) noexcept -> bool
    {
        detail::MetadataEntry entry;
        bool const valid = detail::get_entry(meta, key, detail::MetadataEntryType::StringUTF8, entry);

        if (valid && entry.data_count == 0)
        {
            char8_t const* string_beg = reinterpret_cast<char8_t const*>(meta._additional_data.location) + entry.value_buffer.offset;
            result = ice::Utf8String{ string_beg, entry.value_buffer.size };
        }

        return valid;
    }


    auto meta_read_bool_array(
        ice::Metadata const& meta,
        ice::StringID_Arg key,
        ice::pod::Array<bool>& results
    ) noexcept -> bool
    {
        detail::MetadataEntry entry;
        bool const valid = detail::get_entry(meta, key, detail::MetadataEntryType::Boolean, entry);

        if (valid && entry.data_count != 0)
        {
            bool const* array_beg = reinterpret_cast<bool const*>(meta._additional_data.location) + entry.value_buffer.offset;
            ice::pod::array::push_back(results, ice::Span<bool const>{ array_beg, entry.value_buffer.size });
        }

        return valid;
    }

    auto meta_read_int32_array(
        ice::Metadata const& meta,
        ice::StringID_Arg key,
        ice::pod::Array<ice::i32>& results
    ) noexcept -> bool
    {
        detail::MetadataEntry entry;
        bool const valid = detail::get_entry(meta, key, detail::MetadataEntryType::Integer, entry);

        if (valid && entry.data_count != 0)
        {
            ice::i32 const* array_beg = reinterpret_cast<ice::i32 const*>(
                ice::memory::ptr_add(meta._additional_data.location, entry.value_buffer.offset)
            );
            ice::pod::array::push_back(results, ice::Span<ice::i32 const>{ array_beg, entry.value_buffer.size });
        }

        return valid;
    }

    auto meta_read_flags_array(
        ice::Metadata const& meta,
        ice::StringID_Arg key,
        ice::pod::Array<ice::ResourceFlags>& results
    ) noexcept -> bool
    {
        detail::MetadataEntry entry;
        bool const valid = detail::get_entry(meta, key, detail::MetadataEntryType::Integer, entry);

        if (valid && entry.data_count != 0)
        {
            ice::ResourceFlags const* array_beg = reinterpret_cast<ice::ResourceFlags const*>(
                ice::memory::ptr_add(meta._additional_data.location, entry.value_buffer.offset)
            );
            ice::pod::array::push_back(results, ice::Span<ice::ResourceFlags const>{ array_beg, entry.value_buffer.size });
        }

        return valid;
    }

    auto meta_read_float_array(
        ice::Metadata const& meta,
        ice::StringID_Arg key,
        ice::pod::Array<ice::f32>& results
    ) noexcept -> bool
    {
        detail::MetadataEntry entry;
        bool const valid = detail::get_entry(meta, key, detail::MetadataEntryType::Float, entry);

        if (valid && entry.data_count != 0)
        {
            ice::f32 const* array_beg = reinterpret_cast<ice::f32 const*>(
                ice::memory::ptr_add(meta._additional_data.location, entry.value_buffer.offset)
            );
            ice::pod::array::push_back(results, ice::Span<ice::f32 const>{ array_beg, entry.value_buffer.size });
        }

        return valid;
    }

    auto meta_read_string_array(
        ice::Metadata const& meta,
        ice::StringID_Arg key,
        ice::pod::Array<ice::String>& results
    ) noexcept -> bool
    {
        detail::MetadataEntry entry;
        bool const valid = detail::get_entry(meta, key, detail::MetadataEntryType::String, entry);

        if (valid && entry.data_count != 0)
        {
            detail::MetadataEntryBuffer const* array_beg = reinterpret_cast<detail::MetadataEntryBuffer const*>(
                ice::memory::ptr_add(meta._additional_data.location, entry.value_buffer.offset)
            );
            ice::Span<detail::MetadataEntryBuffer const> array_entries{ array_beg, entry.value_buffer.size };

            for (detail::MetadataEntryBuffer const& string_buffer : array_entries)
            {
                char const* string_beg = reinterpret_cast<char const*>(meta._additional_data.location) + string_buffer.offset;
                ice::pod::array::push_back(results, ice::String{ string_beg, string_buffer.size });
            }
        }

        return valid;
    }

    auto meta_read_utf8_array(
        ice::Metadata const& meta,
        ice::StringID_Arg key,
        ice::pod::Array<ice::Utf8String>& results
    ) noexcept -> bool
    {
        detail::MetadataEntry entry;
        bool const valid = detail::get_entry(meta, key, detail::MetadataEntryType::StringUTF8, entry);

        if (valid && entry.data_count != 0)
        {
            detail::MetadataEntryBuffer const* array_beg = reinterpret_cast<detail::MetadataEntryBuffer const*>(
                ice::memory::ptr_add(meta._additional_data.location, entry.value_buffer.offset)
            );
            ice::Span<detail::MetadataEntryBuffer const> array_entries{ array_beg, entry.value_buffer.size };

            for (detail::MetadataEntryBuffer const& string_buffer : array_entries)
            {
                char8_t const* string_beg = reinterpret_cast<char8_t const*>(meta._additional_data.location) + string_buffer.offset;
                ice::pod::array::push_back(results, ice::Utf8String{ string_beg, string_buffer.size });
            }
        }

        return valid;
    }


    void meta_set_bool(
        ice::MutableMetadata& meta,
        ice::StringID_Arg key,
        bool value
    ) noexcept
    {
        ice::pod::hash::set(
            meta._meta_entries,
            ice::hash(key.hash_value),
            detail::MetadataEntry{
                .data_type = detail::MetadataEntryType::Boolean,
                .data_count = 0,
                .value_int = static_cast<ice::i32>(value),
            }
        );
    }

    void meta_set_int32(
        ice::MutableMetadata& meta,
        ice::StringID_Arg key,
        ice::i32 value
    ) noexcept
    {
        ice::pod::hash::set(
            meta._meta_entries,
            ice::hash(key.hash_value),
            detail::MetadataEntry{
                .data_type = detail::MetadataEntryType::Integer,
                .data_count = 0,
                .value_int = value,
            }
        );
    }

    void meta_set_float(
        ice::MutableMetadata& meta,
        ice::StringID_Arg key,
        ice::f32 value
    ) noexcept
    {
        ice::pod::hash::set(
            meta._meta_entries,
            ice::hash(key.hash_value),
            detail::MetadataEntry{
                .data_type = detail::MetadataEntryType::Float,
                .data_count = 0,
                .value_float = value,
            }
        );
    }

    void meta_set_string(
        ice::MutableMetadata& meta,
        ice::StringID_Arg key,
        ice::String value
    ) noexcept
    {
        void const* str_dest = ice::buffer::append(
            meta._additional_data,
            ice::data_view(ice::string::data(value), ice::string::size(value))
        );

        ice::u32 const str_offset = ice::memory::ptr_distance(
            ice::buffer::data(meta._additional_data),
            str_dest
        );

        ice::pod::hash::set(
            meta._meta_entries,
            ice::hash(key.hash_value),
            detail::MetadataEntry{
                .data_type = detail::MetadataEntryType::String,
                .data_count = 0,
                .value_buffer = detail::MetadataEntryBuffer{
                    .offset = static_cast<ice::u16>(str_offset),
                    .size = static_cast<ice::u16>(ice::string::size(value))
                },
            }
        );
    }

    void meta_set_utf8(
        ice::MutableMetadata& meta,
        ice::StringID_Arg key,
        ice::Utf8String value
    ) noexcept
    {
        void const* str_dest = ice::buffer::append(
            meta._additional_data,
            ice::data_view(ice::string::data(value), ice::string::size(value))
        );

        ice::u32 const str_offset = ice::memory::ptr_distance(
            ice::buffer::data(meta._additional_data),
            str_dest
        );

        ice::pod::hash::set(
            meta._meta_entries,
            ice::hash(key.hash_value),
            detail::MetadataEntry{
                .data_type = detail::MetadataEntryType::StringUTF8,
                .data_count = 0,
                .value_buffer = detail::MetadataEntryBuffer{
                    .offset = static_cast<ice::u16>(str_offset),
                    .size = static_cast<ice::u16>(ice::string::size(value))
                },
            }
        );
    }


    void meta_set_bool_array(
        ice::MutableMetadata& meta,
        ice::StringID_Arg key,
        ice::Span<bool const> values
    ) noexcept
    {
        if (values.empty() == false)
        {
            void const* array_dest = ice::buffer::append(
                meta._additional_data,
                ice::data_view(values.data(), values.size_bytes())
            );

            ice::u32 const array_offset = ice::memory::ptr_distance(
                ice::buffer::data(meta._additional_data),
                array_dest
            );

            ice::pod::hash::set(
                meta._meta_entries,
                ice::hash(key.hash_value),
                detail::MetadataEntry{
                    .data_type = detail::MetadataEntryType::Boolean,
                    .data_count = static_cast<ice::u16>(values.size()),
                    .value_buffer = detail::MetadataEntryBuffer{
                        .offset = static_cast<ice::u16>(array_offset),
                        .size = static_cast<ice::u16>(values.size())
                    },
                }
            );
        }
    }

    void meta_set_int32_array(
        ice::MutableMetadata& meta,
        ice::StringID_Arg key,
        ice::Span<ice::i32 const> values
    ) noexcept
    {
        if (values.empty() == false)
        {
            void const* array_dest = ice::buffer::append(
                meta._additional_data,
                ice::data_view(values.data(), values.size_bytes())
            );

            ice::u32 const array_offset = ice::memory::ptr_distance(
                ice::buffer::data(meta._additional_data),
                array_dest
            );

            ice::pod::hash::set(
                meta._meta_entries,
                ice::hash(key.hash_value),
                detail::MetadataEntry{
                    .data_type = detail::MetadataEntryType::Integer,
                    .data_count = static_cast<ice::u16>(values.size()),
                    .value_buffer = detail::MetadataEntryBuffer{
                        .offset = static_cast<ice::u16>(array_offset),
                        .size = static_cast<ice::u16>(values.size())
                    },
                }
            );
        }
    }

    void meta_set_float_array(
        ice::MutableMetadata& meta,
        ice::StringID_Arg key,
        ice::Span<ice::f32 const> values
    ) noexcept
    {
        if (values.empty() == false)
        {
            void const* array_dest = ice::buffer::append(
                meta._additional_data,
                ice::data_view(values.data(), values.size_bytes())
            );

            ice::u32 const array_offset = ice::memory::ptr_distance(
                ice::buffer::data(meta._additional_data),
                array_dest
            );

            ice::pod::hash::set(
                meta._meta_entries,
                ice::hash(key.hash_value),
                detail::MetadataEntry{
                    .data_type = detail::MetadataEntryType::Float,
                    .data_count = static_cast<ice::u16>(values.size()),
                    .value_buffer = detail::MetadataEntryBuffer{
                        .offset = static_cast<ice::u16>(array_offset),
                        .size = static_cast<ice::u16>(values.size())
                    },
                }
            );
        }
    }

    void meta_set_string_array(
        ice::MutableMetadata& meta,
        ice::StringID_Arg key,
        ice::Span<ice::String const> values
    ) noexcept
    {
        if (values.empty() == false)
        {

            ice::u32 required_capacity = 0;
            required_capacity += ice::buffer::size(meta._additional_data);
            required_capacity += sizeof(detail::MetadataEntryBuffer) * values.size() + alignof(detail::MetadataEntryBuffer);
            for (ice::String value : values)
            {
                required_capacity += ice::string::size(value) + 1;
            }

            ice::buffer::reserve(meta._additional_data, required_capacity);

            void* array_dest = ice::buffer::append(
                meta._additional_data,
                nullptr,
                sizeof(detail::MetadataEntryBuffer) * values.size(),
                alignof(detail::MetadataEntryBuffer)
            );

            ice::u32 const array_offset = ice::memory::ptr_distance(
                ice::buffer::data(meta._additional_data),
                array_dest
            );

            auto* entries = reinterpret_cast<detail::MetadataEntryBuffer*>(array_dest);

            for (ice::String value : values)
            {
                void const* str_dest = ice::buffer::append(
                    meta._additional_data,
                    ice::data_view(ice::string::data(value), ice::string::size(value))
                );

                entries->size = ice::string::size(value);
                entries->offset = ice::memory::ptr_distance(
                    ice::buffer::data(meta._additional_data),
                    str_dest
                );
                entries += 1;
            }

            ice::pod::hash::set(
                meta._meta_entries,
                ice::hash(key.hash_value),
                detail::MetadataEntry{
                    .data_type = detail::MetadataEntryType::String,
                    .data_count = static_cast<ice::u16>(values.size()),
                    .value_buffer = detail::MetadataEntryBuffer{
                        .offset = static_cast<ice::u16>(array_offset),
                        .size = static_cast<ice::u16>(values.size())
                    },
                }
            );
        }
    }

    void meta_set_utf8_array(
        ice::MutableMetadata& meta,
        ice::StringID_Arg key,
        ice::Span<ice::Utf8String const> values
    ) noexcept
    {
        if (values.empty() == false)
        {

            ice::u32 required_capacity = 0;
            required_capacity += ice::buffer::size(meta._additional_data);
            required_capacity += sizeof(detail::MetadataEntryBuffer) * values.size() + alignof(detail::MetadataEntryBuffer);
            for (ice::Utf8String value : values)
            {
                required_capacity += ice::string::size(value) + 1;
            }

            ice::buffer::reserve(meta._additional_data, required_capacity);

            void* array_dest = ice::buffer::append(
                meta._additional_data,
                nullptr,
                sizeof(detail::MetadataEntryBuffer) * values.size(),
                alignof(detail::MetadataEntryBuffer)
            );

            ice::u32 const array_offset = ice::memory::ptr_distance(
                ice::buffer::data(meta._additional_data),
                array_dest
            );

            auto* entries = reinterpret_cast<detail::MetadataEntryBuffer*>(array_dest);

            for (ice::Utf8String value : values)
            {
                void const* str_dest = ice::buffer::append(
                    meta._additional_data,
                    ice::data_view(ice::string::data(value), ice::string::size(value))
                );

                entries->size = ice::string::size(value);
                entries->offset = ice::memory::ptr_distance(
                    ice::buffer::data(meta._additional_data),
                    str_dest
                );
                entries += 1;
            }

            ice::pod::hash::set(
                meta._meta_entries,
                ice::hash(key.hash_value),
                detail::MetadataEntry{
                    .data_type = detail::MetadataEntryType::StringUTF8,
                    .data_count = static_cast<ice::u16>(values.size()),
                    .value_buffer = detail::MetadataEntryBuffer{
                        .offset = static_cast<ice::u16>(array_offset),
                        .size = static_cast<ice::u16>(values.size())
                    },
                }
            );
        }
    }


    void meta_deserialize(ice::Data data, ice::MutableMetadata& meta) noexcept
    {
        char const* it = reinterpret_cast<char const*>(data.location);
        if (it != nullptr)
        {
            ice::String const loaded_header{ it, 4 };

            if (loaded_header == ice::Constant_FileHeader_MetadataFile)
            {
                detail::deserialize_binary_meta(ice::Data{ it + 4, data.size - 4, data.alignment }, meta);
            }
            else
            {
                detail::deserialize_json_meta(data, meta);
            }
        }
    }

    void meta_store(ice::Metadata const& meta, ice::Buffer& buffer) noexcept
    {
        ice::u32 const hash_count = ice::pod::array::capacity(meta._meta_entries._hash);
        ice::u32 const value_count = ice::pod::array::size(meta._meta_entries._data);

        ice::u32 const hash_size = sizeof(*meta._meta_entries._hash._data);
        ice::u32 const value_size = sizeof(*meta._meta_entries._data._data);

        ice::u32 const static_metadata_size = 4 // Header
            + sizeof(ice::u32) * 5 // Sizes
            + hash_size * hash_count // Hash values
            + value_size * value_count // Data values
            + 16; // Offset for alignments

        {
            ice::buffer::set_capacity_aligned(buffer, static_metadata_size, 8);
            ice::buffer::append(buffer, detail::Constant_FileHeaderData_MetadataFile);
            ice::buffer::append(buffer, &hash_count, sizeof(hash_count), 1);
            ice::buffer::append(buffer, &value_count, sizeof(value_count), 1);

            ice::u32* const hash_offset = reinterpret_cast<ice::u32*>(
                ice::buffer::append(buffer, nullptr, sizeof(ice::u32), 1)
            );
            ice::u32* const value_offset = reinterpret_cast<ice::u32*>(
                ice::buffer::append(buffer, nullptr, sizeof(ice::u32), 1)
            );
            ice::u32* const data_offset = reinterpret_cast<ice::u32*>(
                ice::buffer::append(buffer, nullptr, sizeof(ice::u32), 1)
            );

            void const* const hash_location = ice::buffer::append(
                buffer,
                meta._meta_entries._hash._data,
                meta._meta_entries._hash._size * hash_size,
                8
            );
            void const* const value_location = ice::buffer::append(
                buffer,
                meta._meta_entries._data._data,
                meta._meta_entries._data._size * value_size,
                8
            );

            *hash_offset = ice::memory::ptr_distance(ice::buffer::data(buffer), hash_location);
            *value_offset = ice::memory::ptr_distance(ice::buffer::data(buffer), value_location);

            *data_offset = ice::buffer::size(buffer);

            // #todo assert
            // IS_ASSERT(*data_offset <= static_metadata_size, "Size for static metadata was inproperly calculated!");
            ice::buffer::append(buffer, meta._additional_data);
        }
    }

    auto meta_load(ice::Data data) noexcept -> ice::Metadata
    {
        Metadata result_meta{ };

        if (data.location == nullptr)
        {
            return result_meta;
        }

        char const* it = reinterpret_cast<char const*>(data.location);

        ice::String const head{ it, 4 };
        ICE_ASSERT(ice::string::equals(head, ice::Constant_FileHeader_MetadataFile), "Invalid IceShard meta header!");
        it += 4;

        ice::u32 const hash_count = *reinterpret_cast<ice::u32 const*>(it + 0);
        ice::u32 const value_count = *reinterpret_cast<ice::u32 const*>(it + 4);
        it += 8;

        ice::u32 const hash_offset = *reinterpret_cast<ice::u32 const*>(it + 0);
        ice::u32 const value_offset = *reinterpret_cast<ice::u32 const*>(it + 4);
        ice::u32 const data_offset = *reinterpret_cast<ice::u32 const*>(it + 8);

        [[maybe_unused]]
        ice::u32 const hash_type_size = sizeof(*result_meta._meta_entries._hash._data);

        [[maybe_unused]]
        ice::u32 const value_type_size = sizeof(*result_meta._meta_entries._data._data);

        {
            void const* hash_it = ice::memory::ptr_add(data.location, hash_offset);

            if constexpr (ice::build::is_release == false)
            {
                void  const* hash_end = ice::memory::ptr_add(hash_it, hash_count * hash_type_size);
                ICE_ASSERT(
                    ice::memory::ptr_distance(data.location, hash_end) < static_cast<ice::i32>(data.size),
                    "Moved past the data buffer!"
                );
            }

            result_meta._meta_entries._hash._data = const_cast<ice::u32*>(
                reinterpret_cast<ice::u32 const*>(hash_it)
            );
            result_meta._meta_entries._hash._capacity = hash_count;
            result_meta._meta_entries._hash._size = hash_count;
        }

        {
            void const* value_it = ice::memory::ptr_add(data.location, value_offset);

            if constexpr (ice::build::is_release == false)
            {
                void const* value_end = ice::memory::ptr_add(value_it, value_count * value_type_size);
                ICE_ASSERT(
                    ice::memory::ptr_distance(data.location, value_end) <= static_cast<ice::i32>(data.size),
                    "Moved past the data buffer!"
                );
            }

            // #todo make a special Hash type for metadata objects, we DO NOT WANT ANY const_cast!!!
            result_meta._meta_entries._data._data = const_cast<ice::pod::Hash<detail::MetadataEntry>::Entry*>(
                reinterpret_cast<ice::pod::Hash<detail::MetadataEntry>::Entry const*>(value_it)
            );
            result_meta._meta_entries._data._capacity = value_count;
            result_meta._meta_entries._data._size = value_count;

            void const* data_it = ice::memory::ptr_add(data.location, data_offset);
            result_meta._additional_data = { data_it, data.size - data_offset };
        }

        return result_meta;
    }

    MutableMetadata::MutableMetadata(ice::Allocator& alloc) noexcept
        : _meta_entries{ alloc }
        , _additional_data{ alloc }
    { }

    MutableMetadata::MutableMetadata(MutableMetadata&& other) noexcept
        : _meta_entries{ ice::move(other._meta_entries) }
        , _additional_data{ ice::move(other._additional_data) }
    { }

    auto MutableMetadata::operator=(MutableMetadata&& other) noexcept -> MutableMetadata&
    {
        if (this != &other)
        {
            _meta_entries = ice::move(other._meta_entries);
            _additional_data = ice::move(other._additional_data);
        }
        return *this;
    }

    MutableMetadata::operator Metadata() const noexcept
    {
        Metadata result_meta{ };
        result_meta._meta_entries._data._data = _meta_entries._data._data;
        result_meta._meta_entries._data._size = _meta_entries._data._size;
        result_meta._meta_entries._data._capacity = _meta_entries._data._capacity;
        result_meta._meta_entries._hash._data = _meta_entries._hash._data;
        result_meta._meta_entries._hash._size = _meta_entries._hash._size;
        result_meta._meta_entries._hash._capacity = _meta_entries._hash._capacity;
        result_meta._additional_data = _additional_data;
        return result_meta;
    }

    Metadata::Metadata() noexcept
        : _meta_entries{ ice::memory::null_allocator() }
        , _additional_data{ }
    { }

    Metadata::Metadata(Metadata&& other) noexcept
        : _meta_entries{ ice::move(other._meta_entries) }
        , _additional_data{ ice::move(other._additional_data) }
    {
    }

    Metadata::Metadata(Metadata const& other) noexcept
        : _meta_entries{ ice::memory::null_allocator() }
        , _additional_data{ }
    {
        *this = other;
    }

    auto Metadata::operator=(Metadata&& other) noexcept -> Metadata&
    {
        if (this != &other)
        {
            _meta_entries = ice::move(other._meta_entries);
            _additional_data = ice::move(other._additional_data);
        }
        return *this;
    }

    auto Metadata::operator=(Metadata const& other) noexcept -> Metadata&
    {
        if (this != &other)
        {
            _meta_entries._data._data = other._meta_entries._data._data;
            _meta_entries._data._size = other._meta_entries._data._size;
            _meta_entries._data._capacity = other._meta_entries._data._capacity;
            _meta_entries._hash._data = other._meta_entries._hash._data;
            _meta_entries._hash._size = other._meta_entries._hash._size;
            _meta_entries._hash._capacity = other._meta_entries._hash._capacity;
            _additional_data = other._additional_data;
        }
        return *this;
    }

} // namespace ice
