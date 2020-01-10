#include <resource/resource_meta.hxx>
#include <core/pod/hash.hxx>
#include <core/debug/assert.hxx>
#include <core/memory.hxx>

#include <rapidjson/document.h>

namespace resource
{
    namespace detail
    {

        auto get_entry(ResourceMetaView const& meta, core::cexpr::stringid_argument_type key, MetaEntryType expected_type) noexcept -> MetaEntry
        {
            auto entry = core::pod::hash::get(meta._meta_entries, static_cast<uint64_t>(key.hash_value), resource::detail::MetaEntry{});
            IS_ASSERT(entry.data_type != resource::detail::MetaEntryType::Invalid, "Data for key {} does not exist!", key);
            IS_ASSERT(entry.data_type == expected_type, "Data types does not match! expected: {}, got:{}", (int)expected_type, (int)entry.data_type);
            return entry;
        }

        auto try_get_entry(ResourceMetaView const& meta, core::cexpr::stringid_argument_type key, MetaEntryType expected_type, MetaEntry& entry) noexcept -> bool
        {
            entry = core::pod::hash::get(meta._meta_entries, static_cast<uint64_t>(key.hash_value), resource::detail::MetaEntry{});
            return entry.data_type == expected_type;
        }

        void deserialize_json_meta_helper(rapidjson::Value const& object, std::string key, ResourceMeta& meta)
        {

            for (auto const& entry : object.GetObject())
            {
                if (entry.value.IsObject())
                {
                    deserialize_json_meta_helper(entry.value, key + entry.name.GetString() + ".", meta);
                }
                else
                {
                    std::string field_key = key.substr(1) + entry.name.GetString();

                    if (entry.value.IsBool())
                    {
                        resource::set_meta_bool(meta, core::cexpr::stringid(field_key), entry.value.GetBool());
                    }
                    else if (entry.value.IsInt())
                    {
                        resource::set_meta_int32(meta, core::cexpr::stringid(field_key), entry.value.GetInt());
                    }
                    else if (entry.value.IsFloat())
                    {
                        resource::set_meta_float(meta, core::cexpr::stringid(field_key), entry.value.GetFloat());
                    }
                    else if (entry.value.IsString())
                    {
                        resource::set_meta_string(meta, core::cexpr::stringid(field_key), entry.value.GetString());
                    }
                    else
                    {
                        IS_ASSERT(false, "Unknown value type in resource meta!");
                    }
                }
            }
        }

        void deserialize_json_meta(core::data_view data, ResourceMeta& meta) noexcept
        {
            rapidjson::Document doc;
            doc.Parse(reinterpret_cast<const char*>(data._data), data._size);
            IS_ASSERT(doc.IsObject(), "The resource metadata is not a valid Json object!");

            deserialize_json_meta_helper(doc.GetObject(), ".", meta);
        }

        void deserialize_binary_meta(core::data_view data, ResourceMeta& meta) noexcept
        {
            char const* it = reinterpret_cast<char const*>(data._data);

            uint32_t const hash_count = *reinterpret_cast<uint32_t const*>(it + 0);
            uint32_t const value_count = *reinterpret_cast<uint32_t const*>(it + 4);
            it += 8;

            uint32_t const hash_offset = *reinterpret_cast<uint32_t const*>(it + 0);
            uint32_t const value_offset = *reinterpret_cast<uint32_t const*>(it + 4);
            uint32_t const data_offset = *reinterpret_cast<uint32_t const*>(it + 8);

            uint32_t const hash_type_size = sizeof(*meta._meta_entries._hash._data);
            uint32_t const value_type_size = sizeof(*meta._meta_entries._data._data);

            {
                auto hash_it = core::memory::utils::pointer_add(data._data, hash_offset);

                if constexpr (core::build::is_release == false)
                {
                    auto const hash_end = core::memory::utils::pointer_add(hash_it, hash_count * hash_type_size);
                    IS_ASSERT(core::memory::utils::pointer_distance(data._data, hash_end) < static_cast<int32_t>(data._size), "Moved past the data buffer!");
                }

                core::pod::array::resize(meta._meta_entries._hash, hash_count);
                memcpy(meta._meta_entries._hash._data, hash_it, hash_count * hash_type_size);
            }

            {
                auto value_it = core::memory::utils::pointer_add(data._data, value_offset);

                if constexpr (core::build::is_release == false)
                {
                    auto const value_end = core::memory::utils::pointer_add(value_it, hash_count * hash_type_size);
                    IS_ASSERT(core::memory::utils::pointer_distance(data._data, value_end) < static_cast<int32_t>(data._size), "Moved past the data buffer!");
                }

                core::pod::array::resize(meta._meta_entries._data, value_count);
                memcpy(meta._meta_entries._data._data, value_it, value_count * value_type_size);
            }

            {
                auto data_it = core::memory::utils::pointer_add(data._data, data_offset);
                uint32_t const remaining_size = data._size - data_offset;
                core::buffer::reserve(meta._additional_data, remaining_size);
                core::buffer::append(meta._additional_data, data_it, remaining_size);
            }
        }


    } // namespace detail

    ResourceMeta::ResourceMeta(core::allocator& alloc) noexcept
        : _meta_entries{ alloc }
        , _additional_data{ alloc }
    {
    }

    void serialize_meta(ResourceMeta const& meta, core::Buffer& buffer) noexcept
    {
        uint32_t const hash_count = meta._meta_entries._hash._capacity;
        uint32_t const value_count = meta._meta_entries._data._size;

        uint32_t const hash_size = sizeof(*meta._meta_entries._hash._data);
        uint32_t const value_size = sizeof(*meta._meta_entries._data._data);

        uint32_t const static_metadata_size = 4 + sizeof(uint32_t) * 5 + hash_size * hash_count + value_size * value_count + 16 /* for offset misses */;
        uint32_t const offset_stub = 0;

        // Minimal buffer size.
        core::buffer::reserve(buffer, static_metadata_size);
        core::buffer::append(buffer, "ISAD", 4);
        core::buffer::append(buffer, &hash_count, sizeof(hash_count));
        core::buffer::append(buffer, &value_count, sizeof(value_count));

        auto* const hash_offset = reinterpret_cast<uint32_t*>(core::buffer::append(buffer, &offset_stub, sizeof(offset_stub)));
        auto* const value_offset = reinterpret_cast<uint32_t*>(core::buffer::append(buffer, &offset_stub, sizeof(offset_stub)));
        auto* const data_offset = reinterpret_cast<uint32_t*>(core::buffer::append(buffer, &offset_stub, sizeof(offset_stub)));

        auto const* const hash_location = core::buffer::append_aligned(buffer, { meta._meta_entries._hash._data, meta._meta_entries._hash._size * hash_size, 8 });
        auto const* const value_location = core::buffer::append_aligned(buffer, { meta._meta_entries._data._data, meta._meta_entries._data._size * value_size, 8 });

        *hash_offset = core::memory::utils::pointer_distance(core::buffer::begin(buffer), hash_location);
        *value_offset = core::memory::utils::pointer_distance(core::buffer::begin(buffer), value_location);

        *data_offset = core::buffer::size(buffer);
        IS_ASSERT(*data_offset <= static_metadata_size, "Size for static metadata was inproperly calculated!");
        core::buffer::append(buffer, meta._additional_data);
    }

    void deserialize_meta(core::data_view data, ResourceMeta& meta) noexcept
    {
        char const* it = reinterpret_cast<char const*>(data._data);
        if (it != nullptr)
        {
            core::StringView head{ it, 4 };
            if (core::string::equals(head, "ISAD"))
            {
                detail::deserialize_binary_meta({ it + 4, data._size - 4 }, meta);
            }
            else
            {
                detail::deserialize_json_meta(data, meta);
            }
        }
    }

    auto create_meta_view(ResourceMeta const& meta) noexcept -> ResourceMetaView const
    {
        ResourceMetaView result_meta{ { core::memory::globals::null_allocator() } };
        result_meta._meta_entries._data._data = meta._meta_entries._data._data;
        result_meta._meta_entries._data._size = meta._meta_entries._data._size;
        result_meta._meta_entries._data._capacity = meta._meta_entries._data._capacity;
        result_meta._meta_entries._hash._data = meta._meta_entries._hash._data;
        result_meta._meta_entries._hash._size = meta._meta_entries._hash._size;
        result_meta._meta_entries._hash._capacity = meta._meta_entries._hash._capacity;
        result_meta._additional_data = meta._additional_data;
        return result_meta;
    }

    void set_meta_bool(ResourceMeta& meta, core::cexpr::stringid_argument_type key, bool value) noexcept
    {
        detail::MetaEntry entry{ detail::MetaEntryType::Boolean };
        entry.value_int = static_cast<int32_t>(value);
        core::pod::hash::set(meta._meta_entries, static_cast<uint64_t>(key.hash_value), entry);
    }

    void set_meta_int32(ResourceMeta& meta, core::cexpr::stringid_argument_type key, int32_t value) noexcept
    {
        detail::MetaEntry entry{ detail::MetaEntryType::Integer };
        entry.value_int = value;
        core::pod::hash::set(meta._meta_entries, static_cast<uint64_t>(key.hash_value), entry);
    }

    void set_meta_float(ResourceMeta& meta, core::cexpr::stringid_argument_type key, float value) noexcept
    {
        detail::MetaEntry entry{ detail::MetaEntryType::Float };
        entry.value_float = value;
        core::pod::hash::set(meta._meta_entries, static_cast<uint64_t>(key.hash_value), entry);
    }

    void set_meta_string(ResourceMeta& meta, core::cexpr::stringid_argument_type key, core::StringView value) noexcept
    {
        detail::MetaEntry entry{ detail::MetaEntryType::String };
        void const* str_dest = core::buffer::append(meta._additional_data, core::string::data(value), core::string::size(value));
        uint32_t const str_offset = core::memory::utils::pointer_distance(core::buffer::begin(meta._additional_data), str_dest);

        entry.value_buffer = { str_offset, core::string::size(value) };
        core::pod::hash::set(meta._meta_entries, static_cast<uint64_t>(key.hash_value), entry);
    }

    //////////////////////////////////////////////////////////////////////////

    void store_meta_view(ResourceMetaView const& meta, core::Buffer& buffer) noexcept
    {
        uint32_t const hash_count = meta._meta_entries._hash._capacity;
        uint32_t const value_count = meta._meta_entries._data._size;

        uint32_t const hash_size = sizeof(*meta._meta_entries._hash._data);
        uint32_t const value_size = sizeof(*meta._meta_entries._data._data);

        uint32_t const static_metadata_size = 4 + sizeof(uint32_t) * 5 + hash_size * hash_count + value_size * value_count + 16 /* for offset misses */;
        uint32_t const offset_stub = 0;

        core::buffer::reserve(buffer, static_metadata_size);
        core::buffer::append(buffer, "ISAD", 4);
        core::buffer::append(buffer, &hash_count, sizeof(hash_count));
        core::buffer::append(buffer, &value_count, sizeof(value_count));

        auto* const hash_offset = reinterpret_cast<uint32_t*>(core::buffer::append(buffer, &offset_stub, sizeof(offset_stub)));
        auto* const value_offset = reinterpret_cast<uint32_t*>(core::buffer::append(buffer, &offset_stub, sizeof(offset_stub)));
        auto* const data_offset = reinterpret_cast<uint32_t*>(core::buffer::append(buffer, &offset_stub, sizeof(offset_stub)));

        auto const* const hash_location = core::buffer::append_aligned(buffer, { meta._meta_entries._hash._data, meta._meta_entries._hash._size * hash_size, 8 });
        auto const* const value_location = core::buffer::append_aligned(buffer, { meta._meta_entries._data._data, meta._meta_entries._data._size * value_size, 8 });

        *hash_offset = core::memory::utils::pointer_distance(core::buffer::begin(buffer), hash_location);
        *value_offset = core::memory::utils::pointer_distance(core::buffer::begin(buffer), value_location);

        *data_offset = core::buffer::size(buffer);
        IS_ASSERT(*data_offset <= static_metadata_size, "Size for static metadata was inproperly calculated!");
        core::buffer::append(buffer, meta._additional_data);
    }

    auto load_meta_view(core::data_view data) noexcept -> ResourceMetaView const
    {
        ResourceMetaView result_meta{ { core::memory::globals::null_allocator() } };

        if (data._data == nullptr)
        {
            return result_meta;
        }

        char const* it = reinterpret_cast<char const*>(data._data);

        core::StringView head{ it, 4 };
        IS_ASSERT(core::string::equals(head, "ISAD"), "Invalid IceShard meta header!");
        it += 4;

        uint32_t const hash_count = *reinterpret_cast<uint32_t const*>(it + 0);
        uint32_t const value_count = *reinterpret_cast<uint32_t const*>(it + 4);
        it += 8;

        uint32_t const hash_offset = *reinterpret_cast<uint32_t const*>(it + 0);
        uint32_t const value_offset = *reinterpret_cast<uint32_t const*>(it + 4);
        uint32_t const data_offset = *reinterpret_cast<uint32_t const*>(it + 8);

        uint32_t const hash_type_size = sizeof(*result_meta._meta_entries._hash._data);
        uint32_t const value_type_size = sizeof(*result_meta._meta_entries._data._data);

        {
            auto hash_it = core::memory::utils::pointer_add(data._data, hash_offset);

            if constexpr (core::build::is_release == false)
            {
                auto const hash_end = core::memory::utils::pointer_add(hash_it, hash_count * hash_type_size);
                IS_ASSERT(core::memory::utils::pointer_distance(data._data, hash_end) < static_cast<int32_t>(data._size), "Moved past the data buffer!");
            }

            result_meta._meta_entries._hash._data = const_cast<uint32_t*>(
                reinterpret_cast<uint32_t const*>(hash_it)
            );
            result_meta._meta_entries._hash._capacity = hash_count;
            result_meta._meta_entries._hash._size = hash_count;
        }

        {
            auto value_it = core::memory::utils::pointer_add(data._data, value_offset);

            if constexpr (core::build::is_release == false)
            {
                auto const value_end = core::memory::utils::pointer_add(value_it, hash_count * hash_type_size);
                IS_ASSERT(core::memory::utils::pointer_distance(data._data, value_end) < static_cast<int32_t>(data._size), "Moved past the data buffer!");
            }

            result_meta._meta_entries._data._data = const_cast<core::pod::Hash<detail::MetaEntry>::Entry*>(
                reinterpret_cast<core::pod::Hash<detail::MetaEntry>::Entry const*>(value_it)
            );
            result_meta._meta_entries._data._capacity = hash_count;
            result_meta._meta_entries._data._size = hash_count;
        }

        {
            auto data_it = core::memory::utils::pointer_add(data._data, data_offset);
            result_meta._additional_data = { data_it, data._size - data_offset };
        }

        return result_meta;
    }

    auto get_meta_bool(ResourceMetaView const& meta, core::cexpr::stringid_argument_type key) noexcept -> bool
    {
        auto entry = get_entry(meta, key, detail::MetaEntryType::Boolean);
        return entry.value_int != 0;
    }

    auto get_meta_int32(ResourceMetaView const& meta, core::cexpr::stringid_argument_type key) noexcept -> int32_t
    {
        auto entry = get_entry(meta, key, detail::MetaEntryType::Integer);
        return entry.value_int;
    }

    auto get_meta_float(ResourceMetaView const& meta, core::cexpr::stringid_argument_type key) noexcept -> float
    {
        auto entry = get_entry(meta, key, detail::MetaEntryType::Float);
        return entry.value_float;
    }

    auto get_meta_string(ResourceMetaView const& meta, core::cexpr::stringid_argument_type key) noexcept -> core::StringView
    {
        auto const entry = get_entry(meta, key, detail::MetaEntryType::String);
        auto const string_beg = reinterpret_cast<char const*>(meta._additional_data._data) + entry.value_buffer.offset;
        return { string_beg, entry.value_buffer.size };
    }

    auto get_meta_bool(ResourceMetaView const& meta, core::cexpr::stringid_argument_type key, bool& result) noexcept -> bool
    {
        detail::MetaEntry entry;
        if (try_get_entry(meta, key, detail::MetaEntryType::Boolean, entry))
        {
            result = entry.value_int != 0;
        }
        return entry.data_type == detail::MetaEntryType::Boolean;
    }

    auto get_meta_int32(ResourceMetaView const& meta, core::cexpr::stringid_argument_type key, int32_t& result) noexcept -> bool
    {
        detail::MetaEntry entry;
        if (try_get_entry(meta, key, detail::MetaEntryType::Integer, entry))
        {
            result = entry.value_int;
        }
        return entry.data_type == detail::MetaEntryType::Integer;
    }

    auto get_meta_float(ResourceMetaView const& meta, core::cexpr::stringid_argument_type key, float& result) noexcept -> bool
    {
        detail::MetaEntry entry;
        if (try_get_entry(meta, key, detail::MetaEntryType::Float, entry))
        {
            result = entry.value_float;
        }
        return entry.data_type == detail::MetaEntryType::Float;
    }

    auto get_meta_string(ResourceMetaView const& meta, core::cexpr::stringid_argument_type key, core::StringView& result) noexcept -> bool
    {
        detail::MetaEntry entry;
        if (try_get_entry(meta, key, detail::MetaEntryType::String, entry))
        {
            char const* string_beg = reinterpret_cast<char const*>(meta._additional_data._data) + entry.value_buffer.offset;
            result = core::StringView{ string_beg, entry.value_buffer.size };
        }
        return entry.data_type == detail::MetaEntryType::String;
    }

} // namespace asset
