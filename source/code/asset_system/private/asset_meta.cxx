#include <asset_system/asset_meta.hxx>
#include <core/pod/hash.hxx>
#include <core/debug/assert.hxx>
#include <core/memory.hxx>

namespace asset
{
    namespace detail
    {

        auto get_entry(AsssetMeta const& meta, core::cexpr::stringid_argument_type key, MetaEntryType expected_type) noexcept -> MetaEntry
        {
            auto entry = core::pod::hash::get(meta.meta_entries, static_cast<uint64_t>(key.hash_value), asset::detail::MetaEntry{});
            IS_ASSERT(entry.data_type != asset::detail::MetaEntryType::Invalid, "Data for key {} does not exist!", key);
            IS_ASSERT(entry.data_type == expected_type, "Data types does not match! expected: {}, got:{}", (int)expected_type, (int)entry.data_type);
            return entry;
        }

    } // namespace detail

    auto get_meta_bool(AsssetMeta const& meta, core::cexpr::stringid_argument_type key) noexcept -> bool
    {
        auto entry = get_entry(meta, key, detail::MetaEntryType::Boolean);
        return entry.value_int != 0;
    }

    auto get_meta_int32(AsssetMeta const& meta, core::cexpr::stringid_argument_type key) noexcept -> int32_t
    {
        auto entry = get_entry(meta, key, detail::MetaEntryType::Integer);
        return entry.value_int;
    }

    auto get_meta_float(AsssetMeta const& meta, core::cexpr::stringid_argument_type key) noexcept -> float
    {
        auto entry = get_entry(meta, key, detail::MetaEntryType::Float);
        return entry.value_float;
    }

    auto get_meta_string(AsssetMeta const& meta, core::cexpr::stringid_argument_type key) noexcept -> core::StringView<>
    {
        auto const entry = get_entry(meta, key, detail::MetaEntryType::String);
        auto const string_beg = reinterpret_cast<char const*>(meta.additional_data._data) + entry.value_buffer.offset;
        return { string_beg, string_beg + entry.value_buffer.size };
    }

    void serialize_meta(AsssetMeta const& meta, core::Buffer& buffer) noexcept
    {
        uint32_t const hash_count = meta.meta_entries._hash._capacity;
        uint32_t const value_count = meta.meta_entries._data._size;

        uint32_t const hash_size = sizeof(*meta.meta_entries._hash._data);
        uint32_t const value_size = sizeof(*meta.meta_entries._data._data);

        uint32_t const static_metadata_size = 4 + sizeof(uint32_t) * 3 + hash_size * hash_count + value_size * value_count;

        // Minimal buffer size.
        core::buffer::reserve(buffer, static_metadata_size);
        core::buffer::append(buffer, "ISAD", 4);
        core::buffer::append(buffer, &static_metadata_size, sizeof(static_metadata_size));
        core::buffer::append(buffer, &hash_count, sizeof(hash_count));
        core::buffer::append(buffer, &value_count, sizeof(value_count));
        core::buffer::append(buffer, meta.meta_entries._hash._data, meta.meta_entries._hash._size * hash_size);
        core::buffer::append(buffer, meta.meta_entries._data._data, meta.meta_entries._data._size * value_size);
        core::buffer::append(buffer, meta.additional_data);
    }

    void set_meta_int32(AsssetMeta & meta, core::cexpr::stringid_argument_type key, int32_t value) noexcept
    {
        detail::MetaEntry entry{ detail::MetaEntryType::Integer };
        entry.value_int = value;
        core::pod::hash::set(meta.meta_entries, static_cast<uint64_t>(key.hash_value), entry);
    }

    void set_meta_float(AsssetMeta & meta, core::cexpr::stringid_argument_type key, float value) noexcept
    {
        detail::MetaEntry entry{ detail::MetaEntryType::Float };
        entry.value_float = value;
        core::pod::hash::set(meta.meta_entries, static_cast<uint64_t>(key.hash_value), entry);
    }

    void deserialize_meta(core::data_view data, AsssetMeta& meta) noexcept
    {
        char const* it = reinterpret_cast<char const*>(data._data);
        char const* const end = it + data._size;

        core::StringView<> head{ it, it + 4 };
        IS_ASSERT(core::string::equals(head, "ISAD"), "Invalid IceShard meta header!");
        it += 4;

        uint32_t static_metadata_size = *reinterpret_cast<uint32_t const*>(it + 0);
        uint32_t hash_count = *reinterpret_cast<uint32_t const*>(it + 4);
        uint32_t value_count = *reinterpret_cast<uint32_t const*>(it + 8);
        it += 12;

        uint32_t const hash_size = sizeof(*meta.meta_entries._hash._data);
        uint32_t const value_size = sizeof(*meta.meta_entries._data._data);

        core::pod::array::resize(meta.meta_entries._hash, hash_count);
        memcpy(meta.meta_entries._hash._data, it, hash_count * hash_size);
        it += hash_count * hash_size;

        core::pod::array::resize(meta.meta_entries._data, value_count);
        memcpy(meta.meta_entries._data._data, it, value_count * value_size);
        it += value_count * value_size;

        IS_ASSERT((it - reinterpret_cast<char const*>(data._data)) == static_metadata_size, "Read invalid number of bytes!");
        IS_ASSERT(it <= end, "Moved past the provided data buffer!");
        uint32_t const remaining_size = static_cast<uint32_t>(end - it);
        meta.additional_data._data = remaining_size > 0 ? it : nullptr;
        meta.additional_data._size = remaining_size;
    }

    auto reinterpret_meta(core::data_view data) noexcept -> AsssetMeta const
    {
        AsssetMeta result_meta{ { core::memory::globals::null_allocator() } };

        char const* it = reinterpret_cast<char const*>(data._data);
        char const* const end = it + data._size;

        core::StringView<> head{ it, it + 4 };
        IS_ASSERT(core::string::equals(head, "ISAD"), "Invalid IceShard meta header!");
        it += 4;

        uint32_t static_metadata_size = *reinterpret_cast<uint32_t const*>(it + 0);
        uint32_t hash_count = *reinterpret_cast<uint32_t const*>(it + 4);
        uint32_t value_count = *reinterpret_cast<uint32_t const*>(it + 8);
        it += 12;

        uint32_t const hash_size = sizeof(*result_meta.meta_entries._hash._data);
        uint32_t const value_size = sizeof(*result_meta.meta_entries._data._data);

        result_meta.meta_entries._hash._capacity = hash_count;
        result_meta.meta_entries._hash._size = hash_count;
        result_meta.meta_entries._hash._data = (uint32_t*)(it);
        it += hash_count * hash_size;

        result_meta.meta_entries._data._capacity = value_count;
        result_meta.meta_entries._data._size = value_count;
        result_meta.meta_entries._data._data = (core::pod::Hash<detail::MetaEntry>::Entry*)(it);
        it += value_count * value_size;

        IS_ASSERT((it - reinterpret_cast<char const*>(data._data)) == static_metadata_size, "Read invalid number of bytes!");
        IS_ASSERT(it <= end, "Moved past the provided data buffer!");
        uint32_t const remaining_size = static_cast<uint32_t>(end - it);
        result_meta.additional_data._data = remaining_size > 0 ? it : nullptr;
        result_meta.additional_data._size = remaining_size;

        return result_meta;
    }

} // namespace asset
