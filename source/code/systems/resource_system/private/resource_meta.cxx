/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/resource_meta.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/mem_allocator_forward.hxx>
#include <ice/mem_allocator_host.hxx>
#include <ice/stringid.hxx>
#include <ice/log_formatters.hxx>
#include <ice/assert.hxx>

ISC_WARNING_PUSH
ISCW_OPERATOR_DEPRECATED_BETWEEN_UNRELATED_ENUMERATIONS(ISCW_OP_DISABLE)
#include <rapidjson/document.h>
#undef assert
ISC_WARNING_POP

namespace ice
{

    namespace detail
    {

        constexpr ice::Data Constant_FileHeaderData_MetadataFile{
            .location = ice::string::begin(Constant_FileHeader_MetadataFile),
            .size = { ice::string::size(Constant_FileHeader_MetadataFile) },
            .alignment = ice::ualign::b_1
        };

        bool get_entry(
            ice::Metadata const& meta,
            ice::StringID_Arg key,
            ice::detail::MetadataEntryType expected_type,
            ice::detail::MetadataEntry& entry_out
        ) noexcept
        {
            entry_out = ice::hashmap::get(
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

            ice::HostAllocator host_alloc{ };
            ice::ForwardAllocator alloc{ host_alloc, { .bucket_size = 1_KiB } };

            if (arr[0].IsBool())
            {
                ice::Array<bool> final_values{ alloc };
                ice::array::reserve(final_values, count);

                for (auto const& value : arr)
                {
                    if (value.IsBool())
                    {
                        ice::array::push_back(final_values, value.GetBool());
                    }
                }

                ice::meta_set_bool_array(meta, key, ice::Span<bool>{ final_values });
            }
            else if (arr[0].IsInt())
            {
                ice::Array<ice::i32> final_values{ alloc };
                ice::array::reserve(final_values, count);

                for (auto const& value : arr)
                {
                    if (value.IsInt())
                    {
                        ice::array::push_back(final_values, value.GetInt());
                    }
                }

                ice::meta_set_int32_array(meta, key, ice::Span<ice::i32>{ final_values });
            }
            else if (arr[0].IsFloat())
            {
                ice::Array<ice::f32> final_values{ alloc };
                ice::array::reserve(final_values, count);

                for (auto const& value : arr)
                {
                    if (value.IsFloat())
                    {
                        ice::array::push_back(final_values, value.GetFloat());
                    }
                }

                ice::meta_set_float_array(meta, key, ice::Span<ice::f32>{ final_values });
            }
            else if (arr[0].IsDouble())
            {
                ice::Array<ice::f32> final_values{ alloc };
                ice::array::reserve(final_values, count);

                for (auto const& value : arr)
                {
                    if (value.IsDouble())
                    {
                        ice::array::push_back(final_values, static_cast<ice::f32>(value.GetDouble()));
                    }
                }

                ice::meta_set_float_array(meta, key, ice::Span<ice::f32>{ final_values });
            }
            else if (arr[0].IsString())
            {
                ice::Array<ice::String> final_values{ alloc };
                ice::array::reserve(final_values, count);

                for (auto const& value : arr)
                {
                    if (value.IsString())
                    {
                        ice::array::push_back(final_values, { value.GetString(), value.GetStringLength() });
                    }
                }

                ice::meta_set_string_array(meta, key, ice::Span<ice::String>{ final_values });
            }
            else
            {
                ICE_ASSERT(false, "Unknown value type in resource meta, key: {}", key);
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
                        ice::meta_set_string(meta, ice::stringid(field_key), { entry.value.GetString(), entry.value.GetStringLength() });
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
            doc.Parse(reinterpret_cast<char const*>(data.location), data.size.value);
            //IS_ASSERT(doc.IsObject(), "The resource metadata is not a valid Json object!");

            deserialize_json_meta_helper(doc.GetObject(), ".", meta);
        }

        void deserialize_binary_meta(ice::Data data, ice::MutableMetadata& meta) noexcept
        {
            ice::Metadata const loaded_meta = ice::meta_load(data);

            using HashEntry = typename decltype(Metadata::_meta_entries)::Entry;
            using HashValue = typename decltype(Metadata::_meta_entries)::ValueType;

            using HashEntryMut = typename decltype(MutableMetadata::_meta_entries)::Entry;
            using HashValueMut = typename decltype(MutableMetadata::_meta_entries)::ValueType;

            static_assert(std::is_same_v<HashEntry, HashEntryMut> && std::is_same_v<HashValue, HashValueMut>);

            // Allocate the same capacity we got from the loaded metadata.
            ice::hashmap::clear(meta._meta_entries);
            ice::hashmap::shrink(meta._meta_entries);
            ice::hashmap::reserve(meta._meta_entries, loaded_meta._meta_entries._capacity);

            // Copy hahes, entries and data
            ice::memcpy(
                meta._meta_entries._hashes,
                loaded_meta._meta_entries._hashes,
                ice::size_of<ice::u32> * loaded_meta._meta_entries._capacity
            );
            ice::memcpy(
                meta._meta_entries._entries,
                loaded_meta._meta_entries._entries,
                ice::size_of<HashEntry> * loaded_meta._meta_entries._count
            );
            ice::memcpy(
                meta._meta_entries._data,
                loaded_meta._meta_entries._data,
                ice::size_of<HashValue> * loaded_meta._meta_entries._count
            );

            // Set the count
            meta._meta_entries._count = loaded_meta._meta_entries._count;
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


    auto meta_read_bool_array(
        ice::Metadata const& meta,
        ice::StringID_Arg key,
        ice::Array<bool>& results
    ) noexcept -> bool
    {
        detail::MetadataEntry entry;
        bool const valid = detail::get_entry(meta, key, detail::MetadataEntryType::Boolean, entry);

        if (valid && entry.data_count != 0)
        {
            bool const* array_beg = reinterpret_cast<bool const*>(meta._additional_data.location) + entry.value_buffer.offset;
            ice::array::push_back(results, ice::Span<bool const>{ array_beg, entry.data_count });
        }

        return valid;
    }

    auto meta_read_int32_array(
        ice::Metadata const& meta,
        ice::StringID_Arg key,
        ice::Array<ice::i32>& results
    ) noexcept -> bool
    {
        detail::MetadataEntry entry;
        bool const valid = detail::get_entry(meta, key, detail::MetadataEntryType::Integer, entry);

        if (valid && entry.data_count != 0)
        {
            ice::i32 const* array_beg = reinterpret_cast<ice::i32 const*>(
                ice::ptr_add(meta._additional_data.location, { entry.value_buffer.offset })
            );
            ice::array::push_back(results, ice::Span<ice::i32 const>{ array_beg, entry.data_count });
        }

        return valid;
    }

    auto meta_read_flags_array(
        ice::Metadata const& meta,
        ice::StringID_Arg key,
        ice::Array<ice::ResourceFlags>& results
    ) noexcept -> bool
    {
        detail::MetadataEntry entry;
        bool const valid = detail::get_entry(meta, key, detail::MetadataEntryType::Integer, entry);

        if (valid && entry.data_count != 0)
        {
            ice::ResourceFlags const* array_beg = reinterpret_cast<ice::ResourceFlags const*>(
                ice::ptr_add(meta._additional_data.location, { entry.value_buffer.offset })
            );
            ice::array::push_back(results, ice::Span<ice::ResourceFlags const>{ array_beg, entry.data_count });
        }

        return valid;
    }

    auto meta_read_float_array(
        ice::Metadata const& meta,
        ice::StringID_Arg key,
        ice::Array<ice::f32>& results
    ) noexcept -> bool
    {
        detail::MetadataEntry entry;
        bool const valid = detail::get_entry(meta, key, detail::MetadataEntryType::Float, entry);

        if (valid && entry.data_count != 0)
        {
            ice::f32 const* array_beg = reinterpret_cast<ice::f32 const*>(
                ice::ptr_add(meta._additional_data.location, { entry.value_buffer.offset })
            );
            ice::array::push_back(results, ice::Span<ice::f32 const>{ array_beg, entry.data_count });
        }

        return valid;
    }

    auto meta_read_string_array(
        ice::Metadata const& meta,
        ice::StringID_Arg key,
        ice::Array<ice::String>& results
    ) noexcept -> bool
    {
        detail::MetadataEntry entry;
        bool const valid = detail::get_entry(meta, key, detail::MetadataEntryType::String, entry);

        if (valid && entry.data_count != 0)
        {
            detail::MetadataEntryBuffer const* array_beg = reinterpret_cast<detail::MetadataEntryBuffer const*>(
                ice::ptr_add(meta._additional_data.location, { entry.value_buffer.offset })
            );
            ice::Span<detail::MetadataEntryBuffer const> array_entries{ array_beg, entry.data_count };

            for (detail::MetadataEntryBuffer const& string_buffer : array_entries)
            {
                char const* string_beg = reinterpret_cast<char const*>(meta._additional_data.location) + string_buffer.offset;
                ice::array::push_back(results, ice::String{ string_beg, string_buffer.size });
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
        ice::hashmap::set(
            meta._meta_entries,
            ice::hash(key),
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
        ice::hashmap::set(
            meta._meta_entries,
            ice::hash(key),
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
        ice::hashmap::set(
            meta._meta_entries,
            ice::hash(key),
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
        ice::Memory const mem = ice::buffer::append_reserve(
            meta._additional_data,
            { ice::usize{ ice::string::size(value) + 1 }, ice::ualign::b_4 }
        );

        ice::memcpy(mem, ice::string::data_view(value));
        *((char*)(mem.location) + ice::string::size(value)) = '\0';

        ice::usize const str_offset = ice::ptr_distance(
            ice::buffer::memory_pointer(meta._additional_data),
            mem.location
        );

        ice::hashmap::set(
            meta._meta_entries,
            ice::hash(key),
            detail::MetadataEntry{
                .data_type = detail::MetadataEntryType::String,
                .data_count = 0,
                .value_buffer = detail::MetadataEntryBuffer{
                    .offset = static_cast<ice::u16>(str_offset.value),
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
        if (ice::span::count(values) > 0)
        {
            ice::Memory const mem = ice::buffer::append_reserve(
                meta._additional_data,
                { ice::span::size_bytes(values), ice::ualign::b_4 }
            );

            ice::memcpy(mem, ice::span::data_view(values));
            ice::usize const offset = ice::ptr_distance(
                ice::buffer::memory_pointer(meta._additional_data),
                mem.location
            );

            ice::hashmap::set(
                meta._meta_entries,
                ice::hash(key),
                detail::MetadataEntry{
                    .data_type = detail::MetadataEntryType::Boolean,
                    .data_count = static_cast<ice::u16>(ice::span::count(values)),
                    .value_buffer = detail::MetadataEntryBuffer{
                        .offset = static_cast<ice::u16>(offset.value),
                        .size = static_cast<ice::u16>(mem.size.value)
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
        if (ice::span::count(values) > 0)
        {
            ice::Memory const mem = ice::buffer::append_reserve(
                meta._additional_data,
                { ice::span::size_bytes(values), ice::align_of<ice::i32> }
            );

            ice::memcpy(mem, ice::span::data_view(values));
            ice::usize const offset = ice::ptr_distance(
                ice::buffer::memory_pointer(meta._additional_data),
                mem.location
            );

            ice::hashmap::set(
                meta._meta_entries,
                ice::hash(key),
                detail::MetadataEntry{
                    .data_type = detail::MetadataEntryType::Integer,
                    .data_count = static_cast<ice::u16>(ice::span::count(values)),
                    .value_buffer = detail::MetadataEntryBuffer{
                        .offset = static_cast<ice::u16>(offset.value),
                        .size = static_cast<ice::u16>(mem.size.value)
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
        if (ice::span::count(values) > 0)
        {
            ice::Memory const mem = ice::buffer::append_reserve(
                meta._additional_data,
                { ice::span::size_bytes(values), ice::align_of<ice::f32> }
            );

            ice::memcpy(mem, ice::span::data_view(values));
            ice::usize const offset = ice::ptr_distance(
                ice::buffer::memory_pointer(meta._additional_data),
                mem.location
            );

            ice::hashmap::set(
                meta._meta_entries,
                ice::hash(key),
                detail::MetadataEntry{
                    .data_type = detail::MetadataEntryType::Float,
                    .data_count = static_cast<ice::u16>(ice::span::count(values)),
                    .value_buffer = detail::MetadataEntryBuffer{
                        .offset = static_cast<ice::u16>(offset.value),
                        .size = static_cast<ice::u16>(mem.size.value)
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
        if (ice::span::count(values) > 0)
        {
            ice::meminfo meta_info = ice::meminfo_of<detail::MetadataEntry> * ice::span::count(values);
            ice::usize strs_offset = meta_info += ice::meminfo{ice::usize{ ice::string::size(values[0]) + 1 }, ice::ualign::b_1};

            for (ice::String value : ice::span::subspan(values, 1))
            {
                meta_info += ice::meminfo{ ice::usize{ ice::string::size(value) + 1 }, ice::ualign::b_1 };
            }

            ice::Memory const mem = ice::buffer::append_reserve(meta._additional_data, meta_info);
            ice::usize const entries_offset = ice::ptr_distance(
                ice::buffer::memory_pointer(meta._additional_data),
                mem.location
            );

            ice::Span<detail::MetadataEntryBuffer> const entries{
                reinterpret_cast<detail::MetadataEntryBuffer*>(mem.location),
                ice::span::count(values)
            };

            ice::ucount idx = 0;
            for (ice::String value : values)
            {
                ice::memcpy(
                    ice::ptr_add(mem.location, strs_offset),
                    ice::string::begin(value),
                    ice::string::size(value)
                );

                entries[idx].size = (ice::u16) ice::string::size(value);
                entries[idx].offset = (ice::u16) (entries_offset + strs_offset).value;

                strs_offset += { ice::string::size(value) + 1 };
                idx += 1;
            }

            ice::hashmap::set(
                meta._meta_entries,
                ice::hash(key),
                detail::MetadataEntry{
                    .data_type = detail::MetadataEntryType::String,
                    .data_count = static_cast<ice::u16>(ice::span::count(values)),
                    .value_buffer = detail::MetadataEntryBuffer{
                        .offset = static_cast<ice::u16>(entries_offset.value),
                        .size = static_cast<ice::u16>(mem.size.value)
                    },
                }
            );
        }
    }

    struct BinaryOffsets
    {
        ice::usize sizes;
        ice::usize hashes;
        ice::usize entries;
        ice::usize values;
        ice::usize data;
    };

    auto meta_meminfo_internal(ice::Metadata const& meta, BinaryOffsets& offsets) noexcept -> ice::meminfo
    {
        using HashEntry = typename decltype(Metadata::_meta_entries)::Entry;
        using HashValue = typename decltype(Metadata::_meta_entries)::ValueType;

        ice::ucount const hash_count = meta._meta_entries._capacity;
        ice::ucount const value_count = meta._meta_entries._count;

        ice::meminfo res = { ice::size_of<char> * 4, ice::ualign::b_8 };
        offsets.sizes = res += ice::meminfo_of<ice::u32> *6;
        offsets.hashes = res += ice::meminfo_of<ice::u32> *hash_count;
        offsets.entries = res += ice::meminfo_of<HashEntry> *value_count;
        offsets.values = res += ice::meminfo_of<HashValue> *value_count;
        offsets.data = res += { meta._additional_data.size, ice::ualign::b_8 };
        return res;
    }

    auto meta_meminfo(ice::Metadata const& meta) noexcept -> ice::meminfo
    {
        ice::BinaryOffsets offsets;
        return meta_meminfo_internal(meta, offsets);
    }

    auto meta_deserialize(ice::Allocator& alloc, ice::Data data) noexcept -> ice::MutableMetadata
    {
        ice::MutableMetadata result{ alloc };
        [[maybe_unused]]
        ice::Result const unused = ice::meta_deserialize_from(result, data);
        return result;
    }

    auto meta_deserialize_from(ice::MutableMetadata& meta, ice::Data data) noexcept -> ice::Expected<ice::ErrorCode>
    {
        char const* it = reinterpret_cast<char const*>(data.location);
        if (it != nullptr)
        {
            ice::String const loaded_header{ it, 4 };
            if (loaded_header == ice::Constant_FileHeader_MetadataFile)
            {
                detail::deserialize_binary_meta(ice::Data{ it, { data.size.value }, data.alignment }, meta);
            }
            else
            {
                detail::deserialize_json_meta(data, meta);
            }
            return ice::S_Success;
        }
        return ice::E_InvalidArgument;
    }

    auto meta_store(ice::Metadata const& meta, ice::Memory out_data) noexcept -> ice::usize
    {
        using HashEntry = typename decltype(Metadata::_meta_entries)::Entry;
        using HashValue = typename decltype(Metadata::_meta_entries)::ValueType;

        ice::ucount const hash_count = meta._meta_entries._capacity;
        ice::ucount const value_count = meta._meta_entries._count;

        ice::BinaryOffsets offsets;
        ice::meminfo const meta_meminfo = meta_meminfo_internal(meta, offsets);
        ice::usize const sizes_offset = offsets.sizes;
        ice::usize const hashes_offset = offsets.hashes;
        ice::usize const entries_offset = offsets.entries;
        ice::usize const values_offset = offsets.values;
        ice::usize const data_offset = offsets.data;

        if (out_data.size < meta_meminfo.size)
        {
            return 0_B;
        }

        ice::ucount const counts[]{
            hash_count,
            value_count,
            ice::ucount(hashes_offset.value),
            ice::ucount(entries_offset.value),
            ice::ucount(values_offset.value),
            ice::ucount(data_offset.value),
        };

        ice::Memory mem = out_data;
        ice::memcpy(mem, detail::Constant_FileHeaderData_MetadataFile);
        ice::memcpy(ice::ptr_add(mem, sizes_offset), ice::data_view(counts));
        ice::memcpy(ice::ptr_add(mem, hashes_offset), { meta._meta_entries._hashes, ice::size_of<ice::u32> * hash_count, ice::align_of<ice::u32> });
        ice::memcpy(ice::ptr_add(mem, entries_offset), { meta._meta_entries._entries, ice::size_of<HashEntry> * value_count, ice::align_of<HashEntry> });
        ice::memcpy(ice::ptr_add(mem, values_offset), { meta._meta_entries._data, ice::size_of<HashValue> * value_count, ice::align_of<HashValue> });

        // TODO: Create a ptr-add that also updates alignment?
        ice::AlignResult<void*> res = ice::align_to(ice::ptr_add(mem.location, data_offset), ice::ualign::b_8);
        mem.alignment = res.alignment;
        mem.location = res.value;
        ice::memcpy(mem, { meta._additional_data.location, meta._additional_data.size, ice::ualign::b_8 });
        return meta_meminfo.size;
    }

    auto meta_save(ice::Metadata const& meta, ice::Allocator& alloc) noexcept -> ice::Memory
    {
        ice::Memory result = alloc.allocate(meta_meminfo(meta));
        ice::usize const write_size = meta_store(meta, result);
        ICE_ASSERT(write_size == result.size, "Failed to properly allocate data for meta data memory!");
        return result;
    }

    auto meta_load(ice::Data data) noexcept -> ice::Metadata
    {
        using HashEntry = typename decltype(Metadata::_meta_entries)::Entry;
        using HashValue = typename decltype(Metadata::_meta_entries)::ValueType;

        Metadata result_meta{ };
        if (data.location == nullptr)
        {
            return result_meta;
        }

        char const* it = reinterpret_cast<char const*>(data.location);

        ice::String const head{ it, 4 };
        ICE_ASSERT(head == ice::Constant_FileHeader_MetadataFile, "Invalid IceShard meta header!");
        it += 4;

        ice::u32 const hash_count = *reinterpret_cast<ice::u32 const*>(it + 0);
        ice::u32 const value_count = *reinterpret_cast<ice::u32 const*>(it + 4);
        it += 8;

        ice::u32 const hashes_offset = *reinterpret_cast<ice::u32 const*>(it + 0);
        ice::u32 const entries_offset = *reinterpret_cast<ice::u32 const*>(it + 4);
        ice::u32 const values_offset = *reinterpret_cast<ice::u32 const*>(it + 8);
        ice::u32 const data_offset = *reinterpret_cast<ice::u32 const*>(it + 12);

        ice::u32 constexpr hash_type_size = sizeof(*result_meta._meta_entries._hashes);
        ice::u32 constexpr entry_type_size = sizeof(*result_meta._meta_entries._entries);
        ice::u32 constexpr value_type_size = sizeof(*result_meta._meta_entries._data);

        {
            void const* hash_it = ice::ptr_add(data.location, { hashes_offset });

            if constexpr (ice::build::is_release == false)
            {
                void const* hash_end = ice::ptr_add(hash_it, { hash_count * hash_type_size });
                ICE_ASSERT(
                    ice::ptr_distance(data.location, hash_end) < data.size,
                    "Moved past the data buffer!"
                );
            }

            result_meta._meta_entries._hashes = reinterpret_cast<ice::u32 const*>(hash_it);
            result_meta._meta_entries._capacity = hash_count;
            result_meta._meta_entries._count = value_count;
        }

        {
            void const* entries_it = ice::ptr_add(data.location, { entries_offset });

            if constexpr (ice::build::is_release == false)
            {
                void const* entries_end = ice::ptr_add(entries_it, { value_count * entry_type_size });
                ICE_ASSERT(
                    ice::ptr_distance(data.location, entries_end) <= data.size,
                    "Moved past the data buffer!"
                );
            }

            result_meta._meta_entries._entries = reinterpret_cast<HashEntry const*>(entries_it);
        }

        {
            void const* value_it = ice::ptr_add(data.location, { values_offset });

            if constexpr (ice::build::is_release == false)
            {
                void const* values_end = ice::ptr_add(value_it, { value_count * value_type_size });
                ICE_ASSERT(
                    ice::ptr_distance(data.location, values_end) <= data.size,
                    "Moved past the data buffer!"
                );
            }

            // #todo make a special Hash type for metadata objects, we DO NOT WANT ANY const_cast!!!
            result_meta._meta_entries._data = reinterpret_cast<HashValue const*>(value_it);

            void const* data_it = ice::ptr_add(data.location, { data_offset });
            result_meta._additional_data = { data_it, { data.size.value - data_offset } };
        }

        return result_meta;
    }

    MutableMetadata::MutableMetadata(ice::Allocator& alloc) noexcept
        : _meta_entries{ alloc }
        , _additional_data{ &alloc }
    { }

    MutableMetadata::MutableMetadata(MutableMetadata&& other) noexcept
        : _meta_entries{ ice::move(other._meta_entries) }
        , _additional_data{ ice::exchange(other._additional_data, {}) }
    { }

    MutableMetadata::~MutableMetadata() noexcept
    {
        ice::buffer::set_capacity(_additional_data, 0_B);
    }

    auto MutableMetadata::operator=(MutableMetadata&& other) noexcept -> MutableMetadata&
    {
        if (this != &other)
        {
            ice::buffer::set_capacity(_additional_data, 0_B);

            _meta_entries = ice::move(other._meta_entries);
            _additional_data = ice::move(other._additional_data);
        }
        return *this;
    }

    MutableMetadata::operator Metadata() const noexcept
    {
        Metadata result_meta{ };
        result_meta._meta_entries._capacity = _meta_entries._capacity;
        result_meta._meta_entries._count = _meta_entries._count;
        result_meta._meta_entries._hashes = _meta_entries._hashes;
        result_meta._meta_entries._entries = _meta_entries._entries;
        result_meta._meta_entries._data = _meta_entries._data;
        result_meta._additional_data = ice::buffer::data_view(_additional_data);
        return result_meta;
    }

    Metadata::Metadata() noexcept
        : _meta_entries{ }
        , _additional_data{ }
    { }

    Metadata::Metadata(Metadata&& other) noexcept
        : _meta_entries{ ice::move(other._meta_entries) }
        , _additional_data{ ice::move(other._additional_data) }
    {
    }

    Metadata::Metadata(Metadata const& other) noexcept
        : _meta_entries{ }
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
            _meta_entries = other._meta_entries;
            _additional_data = other._additional_data;
        }
        return *this;
    }

} // namespace ice
