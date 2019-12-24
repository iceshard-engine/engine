#pragma once
#include <core/base.hxx>
#include <core/string_view.hxx>
#include <core/cexpr/stringid.hxx>
#include <core/pod/collections.hxx>
#include <core/data/view.hxx>
#include <core/data/buffer.hxx>

namespace resource
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

    struct ResourceMeta
    {
        ResourceMeta(core::allocator& alloc) noexcept;
        ~ResourceMeta() noexcept = default;

        core::pod::Hash<detail::MetaEntry> _meta_entries;
        core::Buffer _additional_data;
    };

    struct ResourceMetaView
    {
        core::pod::Hash<detail::MetaEntry> _meta_entries;
        core::data_view _additional_data;
    };


    void serialize_meta(ResourceMeta const& meta, core::Buffer& buffer) noexcept;

    void deserialize_meta(core::data_view data, ResourceMeta& meta) noexcept;

    void set_meta_bool(ResourceMeta& meta, core::cexpr::stringid_argument_type key, bool value) noexcept;

    void set_meta_int32(ResourceMeta& meta, core::cexpr::stringid_argument_type key, int32_t value) noexcept;

    void set_meta_float(ResourceMeta& meta, core::cexpr::stringid_argument_type key, float value) noexcept;

    void set_meta_string(ResourceMeta& meta, core::cexpr::stringid_argument_type key, core::StringView<> value) noexcept;

    auto create_meta_view(ResourceMeta const& meta) noexcept -> ResourceMetaView const;


    void store_meta_view(ResourceMetaView const& meta, core::Buffer& buffer) noexcept;

    auto load_meta_view(core::data_view data) noexcept -> ResourceMetaView const;

    auto get_meta_bool(ResourceMetaView const& meta, core::cexpr::stringid_argument_type key) noexcept -> bool;

    auto get_meta_int32(ResourceMetaView const& meta, core::cexpr::stringid_argument_type key) noexcept -> int32_t;

    auto get_meta_float(ResourceMetaView const& meta, core::cexpr::stringid_argument_type key) noexcept -> float;

    auto get_meta_string(ResourceMetaView const& meta, core::cexpr::stringid_argument_type key) noexcept -> core::StringView<>;

} // namespace asset
