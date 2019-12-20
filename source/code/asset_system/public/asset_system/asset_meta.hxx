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

    struct AssetMeta
    {
        AssetMeta(core::allocator& alloc) noexcept;
        ~AssetMeta() noexcept = default;

        core::pod::Hash<detail::MetaEntry> _meta_entries;
        core::Buffer _additional_data;
    };

    struct AssetMetaView
    {
        core::pod::Hash<detail::MetaEntry> _meta_entries;
        core::data_view _additional_data;
    };


    void serialize_meta(AssetMeta const& meta, core::Buffer& buffer) noexcept;

    void deserialize_meta(core::data_view data, AssetMeta& meta) noexcept;

    void set_meta_bool(AssetMeta& meta, core::cexpr::stringid_argument_type key, bool value) noexcept;

    void set_meta_int32(AssetMeta& meta, core::cexpr::stringid_argument_type key, int32_t value) noexcept;

    void set_meta_float(AssetMeta& meta, core::cexpr::stringid_argument_type key, float value) noexcept;

    void set_meta_string(AssetMeta& meta, core::cexpr::stringid_argument_type key, core::StringView<> value) noexcept;

    auto create_meta_view(AssetMeta const& meta) noexcept -> AssetMetaView const;


    void store_meta_view(AssetMetaView const& meta, core::Buffer& buffer) noexcept;

    auto load_meta_view(core::data_view data) noexcept -> AssetMetaView const;

    auto get_meta_bool(AssetMetaView const& meta, core::cexpr::stringid_argument_type key) noexcept -> bool;

    auto get_meta_int32(AssetMetaView const& meta, core::cexpr::stringid_argument_type key) noexcept -> int32_t;

    auto get_meta_float(AssetMetaView const& meta, core::cexpr::stringid_argument_type key) noexcept -> float;

    auto get_meta_string(AssetMetaView const& meta, core::cexpr::stringid_argument_type key) noexcept -> core::StringView<>;

} // namespace asset
