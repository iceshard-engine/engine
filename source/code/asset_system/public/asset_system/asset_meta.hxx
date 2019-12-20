#pragma once
#include <core/base.hxx>
#include <core/string_view.hxx>
#include <core/cexpr/stringid.hxx>
#include <core/pod/collections.hxx>
#include <core/data/view.hxx>
#include <core/data/buffer.hxx>

namespace asset
{
    namespace detail
    {

        enum class MetaEntryType : int32_t
        {
            Invalid,
            Boolean,
            Integer,
            Float,
            String,
            Buffer,
        };

        struct MetaEntryBuffer
        {
            uint32_t offset;
            uint32_t size;
        };

        struct MetaEntry
        {
            MetaEntryType data_type = MetaEntryType::Invalid;
            union {
                int32_t value_int;
                float value_float;
                MetaEntryBuffer value_buffer;
            };
        };

    } // namespace detail

    struct AsssetMeta
    {
        core::pod::Hash<detail::MetaEntry> meta_entries;
        core::data_view additional_data;
    };

    auto get_meta_bool(AsssetMeta const& meta, core::cexpr::stringid_argument_type key) noexcept -> bool;

    auto get_meta_int32(AsssetMeta const& meta, core::cexpr::stringid_argument_type key) noexcept -> int32_t;

    auto get_meta_float(AsssetMeta const& meta, core::cexpr::stringid_argument_type key) noexcept -> float;

    auto get_meta_string(AsssetMeta const& meta, core::cexpr::stringid_argument_type key) noexcept -> core::StringView<>;

    void serialize_meta(AsssetMeta const& meta, core::Buffer& buffer) noexcept;

    void set_meta_int32(AsssetMeta& meta, core::cexpr::stringid_argument_type key, int32_t value) noexcept;

    void set_meta_float(AsssetMeta& meta, core::cexpr::stringid_argument_type key, float value) noexcept;


    void deserialize_meta(core::data_view data, AsssetMeta& meta) noexcept;

    auto reinterpret_meta(core::data_view data) noexcept -> AsssetMeta const;

} // namespace asset
