/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT
#include "config_impl.hxx"

namespace ice
{

    namespace detail
    {

        void deserialize_binary_config(ice::Data data, ice::MutableConfig& config) noexcept
        {
            ice::Config const loaded_config = ice::config_load(data);

            using HashEntry = typename decltype(Config::_config_entries)::Entry;
            using HashValue = typename decltype(Config::_config_entries)::ValueType;

            using HashEntryMut = typename decltype(MutableConfig::_config_entries)::Entry;
            using HashValueMut = typename decltype(MutableConfig::_config_entries)::ValueType;

            static_assert(std::is_same_v<HashEntry, HashEntryMut> && std::is_same_v<HashValue, HashValueMut>);

            // Allocate the same capacity we got from the loaded config.
            ice::hashmap::clear(config._config_entries);
            ice::hashmap::shrink(config._config_entries);
            ice::hashmap::reserve(config._config_entries, loaded_config._config_entries._capacity);

            // Copy hahes, entries and data
            ice::memcpy(
                config._config_entries._hashes,
                loaded_config._config_entries._hashes,
                ice::size_of<ice::u32> * loaded_config._config_entries._capacity
            );
            ice::memcpy(
                config._config_entries._entries,
                loaded_config._config_entries._entries,
                ice::size_of<HashEntry> * loaded_config._config_entries._count
            );
            ice::memcpy(
                config._config_entries._data,
                loaded_config._config_entries._data,
                ice::size_of<HashValue> * loaded_config._config_entries._count
            );

            // Set the count
            config._config_entries._count = loaded_config._config_entries._count;
        }

    } // namespace detail

    bool config_has_entry(ice::Config const& config, ice::StringID_Arg key) noexcept
    {
        detail::ConfigEntry entry;
        return detail::get_entry(config, key, detail::ConfigEntryType::Invalid, entry) == false;
    }

    auto config_read_bool(
        ice::Config const& config,
        ice::StringID_Arg key,
        bool& result
    ) noexcept -> bool
    {
        detail::ConfigEntry entry;
        bool const valid = detail::get_entry(config, key, detail::ConfigEntryType::Boolean, entry);

        if (valid && entry.data_count == 0)
        {
            result = entry.value_int != 0;
        }

        return valid;
    }

    auto config_read_int32(
        ice::Config const& config,
        ice::StringID_Arg key,
        ice::i32& result
    ) noexcept -> bool
    {
        detail::ConfigEntry entry;
        bool const valid = detail::get_entry(config, key, detail::ConfigEntryType::Integer, entry);

        if (valid && entry.data_count == 0)
        {
            result = entry.value_int;
        }

        return valid;
    }

    auto config_read_float(
        ice::Config const& config,
        ice::StringID_Arg key,
        ice::f32& result
    ) noexcept -> bool
    {
        detail::ConfigEntry entry;
        bool const valid = detail::get_entry(config, key, detail::ConfigEntryType::Float, entry);

        if (valid && entry.data_count == 0)
        {
            result = entry.value_float;
        }

        return valid;
    }

    auto config_read_string(
        ice::Config const& config,
        ice::StringID_Arg key,
        ice::String& result
    ) noexcept -> bool
    {
        detail::ConfigEntry entry;
        bool const valid = detail::get_entry(config, key, detail::ConfigEntryType::String, entry);

        if (valid && entry.data_count == 0)
        {
            char const* string_beg = reinterpret_cast<char const*>(config._additional_data.location) + entry.value_buffer.offset;
            result = ice::String{ string_beg, entry.value_buffer.size };
        }

        return valid;
    }


    auto config_read_bool_array(
        ice::Config const& config,
        ice::StringID_Arg key,
        ice::Array<bool>& results
    ) noexcept -> bool
    {
        detail::ConfigEntry entry;
        bool const valid = detail::get_entry(config, key, detail::ConfigEntryType::Boolean, entry);

        if (valid && entry.data_count != 0)
        {
            bool const* array_beg = reinterpret_cast<bool const*>(config._additional_data.location) + entry.value_buffer.offset;
            ice::array::push_back(results, ice::Span<bool const>{ array_beg, entry.data_count });
        }

        return valid;
    }

    auto config_read_int32_array(
        ice::Config const& config,
        ice::StringID_Arg key,
        ice::Array<ice::i32>& results
    ) noexcept -> bool
    {
        detail::ConfigEntry entry;
        bool const valid = detail::get_entry(config, key, detail::ConfigEntryType::Integer, entry);

        if (valid && entry.data_count != 0)
        {
            ice::i32 const* array_beg = reinterpret_cast<ice::i32 const*>(
                ice::ptr_add(config._additional_data.location, { entry.value_buffer.offset })
            );
            ice::array::push_back(results, ice::Span<ice::i32 const>{ array_beg, entry.data_count });
        }

        return valid;
    }

    auto config_read_float_array(
        ice::Config const& config,
        ice::StringID_Arg key,
        ice::Array<ice::f32>& results
    ) noexcept -> bool
    {
        detail::ConfigEntry entry;
        bool const valid = detail::get_entry(config, key, detail::ConfigEntryType::Float, entry);

        if (valid && entry.data_count != 0)
        {
            ice::f32 const* array_beg = reinterpret_cast<ice::f32 const*>(
                ice::ptr_add(config._additional_data.location, { entry.value_buffer.offset })
            );
            ice::array::push_back(results, ice::Span<ice::f32 const>{ array_beg, entry.data_count });
        }

        return valid;
    }

    auto config_read_string_array(
        ice::Config const& config,
        ice::StringID_Arg key,
        ice::Array<ice::String>& results
    ) noexcept -> bool
    {
        detail::ConfigEntry entry;
        bool const valid = detail::get_entry(config, key, detail::ConfigEntryType::String, entry);

        if (valid && entry.data_count != 0)
        {
            detail::ConfigEntryBuffer const* array_beg = reinterpret_cast<detail::ConfigEntryBuffer const*>(
                ice::ptr_add(config._additional_data.location, { entry.value_buffer.offset })
            );
            ice::Span<detail::ConfigEntryBuffer const> array_entries{ array_beg, entry.data_count };

            for (detail::ConfigEntryBuffer const& string_buffer : array_entries)
            {
                char const* string_beg = reinterpret_cast<char const*>(config._additional_data.location) + string_buffer.offset;
                ice::array::push_back(results, ice::String{ string_beg, string_buffer.size });
            }
        }

        return valid;
    }


    void config_set_bool(
        ice::MutableConfig& config,
        ice::StringID_Arg key,
        bool value
    ) noexcept
    {
        ice::hashmap::set(
            config._config_entries,
            ice::hash(key),
            detail::ConfigEntry{
                .data_type = detail::ConfigEntryType::Boolean,
                .data_count = 0,
                .value_int = static_cast<ice::i32>(value),
            }
        );
    }

    void config_set_int32(
        ice::MutableConfig& config,
        ice::StringID_Arg key,
        ice::i32 value
    ) noexcept
    {
        ice::hashmap::set(
            config._config_entries,
            ice::hash(key),
            detail::ConfigEntry{
                .data_type = detail::ConfigEntryType::Integer,
                .data_count = 0,
                .value_int = value,
            }
        );
    }

    void config_set_float(
        ice::MutableConfig& config,
        ice::StringID_Arg key,
        ice::f32 value
    ) noexcept
    {
        ice::hashmap::set(
            config._config_entries,
            ice::hash(key),
            detail::ConfigEntry{
                .data_type = detail::ConfigEntryType::Float,
                .data_count = 0,
                .value_float = value,
            }
        );
    }

    void config_set_string(
        ice::MutableConfig& config,
        ice::StringID_Arg key,
        ice::String value
    ) noexcept
    {
        ice::Memory const mem = ice::buffer::append_reserve(
            config._additional_data,
            { ice::usize{ ice::string::size(value) + 1 }, ice::ualign::b_4 }
        );

        ice::memcpy(mem, ice::string::data_view(value));
        ice::usize const str_offset = ice::ptr_distance(
            ice::buffer::memory_pointer(config._additional_data),
            mem.location
        );

        ice::hashmap::set(
            config._config_entries,
            ice::hash(key),
            detail::ConfigEntry{
                .data_type = detail::ConfigEntryType::String,
                .data_count = 0,
                .value_buffer = detail::ConfigEntryBuffer{
                    .offset = static_cast<ice::u16>(str_offset.value),
                    .size = static_cast<ice::u16>(ice::string::size(value))
                },
            }
        );
    }


    void config_set_bool_array(
        ice::MutableConfig& config,
        ice::StringID_Arg key,
        ice::Span<bool const> values
    ) noexcept
    {
        if (ice::span::count(values) > 0)
        {
            ice::Memory const mem = ice::buffer::append_reserve(
                config._additional_data,
                { ice::span::size_bytes(values), ice::ualign::b_4 }
            );

            ice::memcpy(mem, ice::span::data_view(values));
            ice::usize const offset = ice::ptr_distance(
                ice::buffer::memory_pointer(config._additional_data),
                mem.location
            );

            ice::hashmap::set(
                config._config_entries,
                ice::hash(key),
                detail::ConfigEntry{
                    .data_type = detail::ConfigEntryType::Boolean,
                    .data_count = static_cast<ice::u16>(ice::span::count(values)),
                    .value_buffer = detail::ConfigEntryBuffer{
                        .offset = static_cast<ice::u16>(offset.value),
                        .size = static_cast<ice::u16>(mem.size.value)
                    },
                }
            );
        }
    }

    void config_set_int32_array(
        ice::MutableConfig& config,
        ice::StringID_Arg key,
        ice::Span<ice::i32 const> values
    ) noexcept
    {
        if (ice::span::count(values) > 0)
        {
            ice::Memory const mem = ice::buffer::append_reserve(
                config._additional_data,
                { ice::span::size_bytes(values), ice::align_of<ice::i32> }
            );

            ice::memcpy(mem, ice::span::data_view(values));
            ice::usize const offset = ice::ptr_distance(
                ice::buffer::memory_pointer(config._additional_data),
                mem.location
            );

            ice::hashmap::set(
                config._config_entries,
                ice::hash(key),
                detail::ConfigEntry{
                    .data_type = detail::ConfigEntryType::Integer,
                    .data_count = static_cast<ice::u16>(ice::span::count(values)),
                    .value_buffer = detail::ConfigEntryBuffer{
                        .offset = static_cast<ice::u16>(offset.value),
                        .size = static_cast<ice::u16>(mem.size.value)
                    },
                }
            );
        }
    }

    void config_set_float_array(
        ice::MutableConfig& config,
        ice::StringID_Arg key,
        ice::Span<ice::f32 const> values
    ) noexcept
    {
        if (ice::span::count(values) > 0)
        {
            ice::Memory const mem = ice::buffer::append_reserve(
                config._additional_data,
                { ice::span::size_bytes(values), ice::align_of<ice::f32> }
            );

            ice::memcpy(mem, ice::span::data_view(values));
            ice::usize const offset = ice::ptr_distance(
                ice::buffer::memory_pointer(config._additional_data),
                mem.location
            );

            ice::hashmap::set(
                config._config_entries,
                ice::hash(key),
                detail::ConfigEntry{
                    .data_type = detail::ConfigEntryType::Float,
                    .data_count = static_cast<ice::u16>(ice::span::count(values)),
                    .value_buffer = detail::ConfigEntryBuffer{
                        .offset = static_cast<ice::u16>(offset.value),
                        .size = static_cast<ice::u16>(mem.size.value)
                    },
                }
            );
        }
    }

    void config_set_string_array(
        ice::MutableConfig& config,
        ice::StringID_Arg key,
        ice::Span<ice::String const> values
    ) noexcept
    {
        if (ice::span::count(values) > 0)
        {
            ice::meminfo config_info = ice::meminfo_of<detail::ConfigEntry> * ice::span::count(values);
            ice::usize strs_offset = config_info += ice::meminfo{ice::usize{ ice::string::size(values[0]) + 1 }, ice::ualign::b_1};

            for (ice::String value : ice::span::subspan(values, 1))
            {
                config_info += ice::meminfo{ ice::usize{ ice::string::size(value) + 1 }, ice::ualign::b_1 };
            }

            ice::Memory const mem = ice::buffer::append_reserve(config._additional_data, config_info);
            ice::usize const entries_offset = ice::ptr_distance(
                ice::buffer::memory_pointer(config._additional_data),
                mem.location
            );

            ice::Span<detail::ConfigEntryBuffer> const entries{
                reinterpret_cast<detail::ConfigEntryBuffer*>(mem.location),
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
                config._config_entries,
                ice::hash(key),
                detail::ConfigEntry{
                    .data_type = detail::ConfigEntryType::String,
                    .data_count = static_cast<ice::u16>(ice::span::count(values)),
                    .value_buffer = detail::ConfigEntryBuffer{
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

    auto config_meminfo_internal(ice::Config const& config, BinaryOffsets& offsets) noexcept -> ice::meminfo
    {
        using HashEntry = typename decltype(Config::_config_entries)::Entry;
        using HashValue = typename decltype(Config::_config_entries)::ValueType;

        ice::ucount const hash_count = config._config_entries._capacity;
        ice::ucount const value_count = config._config_entries._count;

        ice::meminfo res = { ice::size_of<char> * 4, ice::ualign::b_8 };
        offsets.sizes = res += ice::meminfo_of<ice::u32> *6;
        offsets.hashes = res += ice::meminfo_of<ice::u32> *hash_count;
        offsets.entries = res += ice::meminfo_of<HashEntry> *value_count;
        offsets.values = res += ice::meminfo_of<HashValue> *value_count;
        offsets.data = res += { config._additional_data.size, ice::ualign::b_8 };
        return res;
    }

    auto config_meminfo(ice::Config const& config) noexcept -> ice::meminfo
    {
        ice::BinaryOffsets offsets;
        return config_meminfo_internal(config, offsets);
    }

    auto config_deserialize(ice::Allocator& alloc, ice::Data data) noexcept -> ice::MutableConfig
    {
        ice::MutableConfig result{ alloc };
        [[maybe_unused]]
        ice::Result const unused = ice::config_deserialize_from(result, data);
        return result;
    }

    auto config_deserialize_from(ice::MutableConfig& config, ice::Data data) noexcept -> ice::Expected<ice::ErrorCode>
    {
        char const* it = reinterpret_cast<char const*>(data.location);
        if (it != nullptr)
        {
            ice::String const loaded_header{ it, 4 };
            if (loaded_header == ice::Constant_FileHeader_ConfigFile)
            {
                detail::deserialize_binary_config(ice::Data{ it, { data.size.value }, data.alignment }, config);
            }
            else
            {
                detail::deserialize_json_config(data, config);
            }
            return ice::S_Success;
        }
        return ice::E_InvalidArgument;
    }

    auto config_store(ice::Config const& config, ice::Memory out_data) noexcept -> ice::usize
    {
        using HashEntry = typename decltype(Config::_config_entries)::Entry;
        using HashValue = typename decltype(Config::_config_entries)::ValueType;

        ice::ucount const hash_count = config._config_entries._capacity;
        ice::ucount const value_count = config._config_entries._count;

        ice::BinaryOffsets offsets;
        ice::meminfo const config_meminfo = config_meminfo_internal(config, offsets);
        ice::usize const sizes_offset = offsets.sizes;
        ice::usize const hashes_offset = offsets.hashes;
        ice::usize const entries_offset = offsets.entries;
        ice::usize const values_offset = offsets.values;
        ice::usize const data_offset = offsets.data;

        if (out_data.size < config_meminfo.size)
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
        ice::memcpy(mem, detail::Constant_FileHeaderData_ConfigFile);
        ice::memcpy(ice::ptr_add(mem, sizes_offset), ice::data_view(counts));
        ice::memcpy(ice::ptr_add(mem, hashes_offset), { config._config_entries._hashes, ice::size_of<ice::u32> * hash_count, ice::align_of<ice::u32> });
        ice::memcpy(ice::ptr_add(mem, entries_offset), { config._config_entries._entries, ice::size_of<HashEntry> * value_count, ice::align_of<HashEntry> });
        ice::memcpy(ice::ptr_add(mem, values_offset), { config._config_entries._data, ice::size_of<HashValue> * value_count, ice::align_of<HashValue> });

        // TODO: Create a ptr-add that also updates alignment?
        ice::AlignResult<void*> res = ice::align_to(ice::ptr_add(mem.location, data_offset), ice::ualign::b_8);
        mem.alignment = res.alignment;
        mem.location = res.value;
        ice::memcpy(mem, { config._additional_data.location, config._additional_data.size, ice::ualign::b_8 });
        return config_meminfo.size;
    }

    auto config_save(ice::Config const& config, ice::Allocator& alloc) noexcept -> ice::Memory
    {
        ice::Memory result = alloc.allocate(config_meminfo(config));
        ice::usize const write_size = config_store(config, result);
        ICE_ASSERT(write_size == result.size, "Failed to properly allocate data for config data memory!");
        return result;
    }

    auto config_load(ice::Data data) noexcept -> ice::Config
    {
        using HashEntry = typename decltype(Config::_config_entries)::Entry;
        using HashValue = typename decltype(Config::_config_entries)::ValueType;

        Config result_config{ };
        if (data.location == nullptr)
        {
            return result_config;
        }

        char const* it = reinterpret_cast<char const*>(data.location);

        ice::String const head{ it, 4 };
        ICE_ASSERT(head == ice::Constant_FileHeader_ConfigFile, "Invalid IceShard config header!");
        it += 4;

        ice::u32 const hash_count = *reinterpret_cast<ice::u32 const*>(it + 0);
        ice::u32 const value_count = *reinterpret_cast<ice::u32 const*>(it + 4);
        it += 8;

        ice::u32 const hashes_offset = *reinterpret_cast<ice::u32 const*>(it + 0);
        ice::u32 const entries_offset = *reinterpret_cast<ice::u32 const*>(it + 4);
        ice::u32 const values_offset = *reinterpret_cast<ice::u32 const*>(it + 8);
        ice::u32 const data_offset = *reinterpret_cast<ice::u32 const*>(it + 12);

        ice::u32 constexpr hash_type_size = sizeof(*result_config._config_entries._hashes);
        ice::u32 constexpr entry_type_size = sizeof(*result_config._config_entries._entries);
        ice::u32 constexpr value_type_size = sizeof(*result_config._config_entries._data);

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

            result_config._config_entries._hashes = reinterpret_cast<ice::u32 const*>(hash_it);
            result_config._config_entries._capacity = hash_count;
            result_config._config_entries._count = value_count;
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

            result_config._config_entries._entries = reinterpret_cast<HashEntry const*>(entries_it);
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

            // #todo make a special Hash type for config objects, we DO NOT WANT ANY const_cast!!!
            result_config._config_entries._data = reinterpret_cast<HashValue const*>(value_it);

            void const* data_it = ice::ptr_add(data.location, { data_offset });
            result_config._additional_data = { data_it, { data.size.value - data_offset } };
        }

        return result_config;
    }

    MutableConfig::MutableConfig(ice::Allocator& alloc) noexcept
        : _config_entries{ alloc }
        , _additional_data{ &alloc }
    { }

    MutableConfig::MutableConfig(MutableConfig&& other) noexcept
        : _config_entries{ ice::move(other._config_entries) }
        , _additional_data{ ice::exchange(other._additional_data, {}) }
    { }

    MutableConfig::~MutableConfig() noexcept
    {
        ice::buffer::set_capacity(_additional_data, 0_B);
    }

    auto MutableConfig::operator=(MutableConfig&& other) noexcept -> MutableConfig&
    {
        if (this != &other)
        {
            ice::buffer::set_capacity(_additional_data, 0_B);

            _config_entries = ice::move(other._config_entries);
            _additional_data = ice::move(other._additional_data);
        }
        return *this;
    }

    MutableConfig::operator Config() const noexcept
    {
        Config result_config{ };
        result_config._config_entries._capacity = _config_entries._capacity;
        result_config._config_entries._count = _config_entries._count;
        result_config._config_entries._hashes = _config_entries._hashes;
        result_config._config_entries._entries = _config_entries._entries;
        result_config._config_entries._data = _config_entries._data;
        result_config._additional_data = ice::buffer::data_view(_additional_data);
        return result_config;
    }

    Config::Config() noexcept
        : _config_entries{ }
        , _additional_data{ }
    { }

    Config::Config(Config&& other) noexcept
        : _config_entries{ ice::move(other._config_entries) }
        , _additional_data{ ice::move(other._additional_data) }
    {
    }

    Config::Config(Config const& other) noexcept
        : _config_entries{ }
        , _additional_data{ }
    {
        *this = other;
    }

    auto Config::operator=(Config&& other) noexcept -> Config&
    {
        if (this != &other)
        {
            _config_entries = ice::move(other._config_entries);
            _additional_data = ice::move(other._additional_data);
        }
        return *this;
    }

    auto Config::operator=(Config const& other) noexcept -> Config&
    {
        if (this != &other)
        {
            _config_entries = other._config_entries;
            _additional_data = other._additional_data;
        }
        return *this;
    }

} // namespace ice
