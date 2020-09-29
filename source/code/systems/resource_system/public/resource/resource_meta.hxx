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

        ResourceMeta(ResourceMeta&& other) noexcept;
        ResourceMeta(ResourceMeta const& other) noexcept = delete;

        auto operator=(ResourceMeta&&) noexcept -> ResourceMeta&;
        auto operator=(ResourceMeta const&) noexcept -> ResourceMeta& = delete;

        core::pod::Hash<detail::MetaEntry> _meta_entries;
        core::Buffer _additional_data;
    };

    struct ResourceMetaView
    {
        core::pod::Hash<detail::MetaEntry> _meta_entries;
        core::data_view _additional_data;

        ResourceMetaView() noexcept;
        ~ResourceMetaView() noexcept = default;
        ResourceMetaView(ResourceMetaView&& other) noexcept;
        ResourceMetaView(ResourceMetaView const& other) noexcept;
        auto operator=(ResourceMetaView&& other) noexcept -> ResourceMetaView&;
        auto operator=(ResourceMetaView const& other) noexcept -> ResourceMetaView&;
    };

    void copy_meta(ResourceMeta& meta_out, ResourceMetaView const& meta_view) noexcept;

    void serialize_meta(ResourceMeta const& meta, core::Buffer& buffer) noexcept;

    void deserialize_meta(core::data_view data, ResourceMeta& meta) noexcept;

    void set_meta_bool(ResourceMeta& meta, core::stringid_arg_type key, bool value) noexcept;

    void set_meta_int32(ResourceMeta& meta, core::stringid_arg_type key, int32_t value) noexcept;

    void set_meta_float(ResourceMeta& meta, core::stringid_arg_type key, float value) noexcept;

    void set_meta_string(ResourceMeta& meta, core::stringid_arg_type key, core::StringView value) noexcept;

    auto create_meta_view(ResourceMeta const& meta) noexcept->ResourceMetaView const;


    void store_meta_view(ResourceMetaView const& meta, core::Buffer& buffer) noexcept;

    auto load_meta_view(core::data_view data) noexcept->ResourceMetaView const;

    auto get_meta_bool(ResourceMetaView const& meta, core::stringid_arg_type key) noexcept -> bool;

    auto get_meta_int32(ResourceMetaView const& meta, core::stringid_arg_type key) noexcept -> int32_t;

    auto get_meta_float(ResourceMetaView const& meta, core::stringid_arg_type key) noexcept -> float;

    auto get_meta_string(ResourceMetaView const& meta, core::stringid_arg_type key) noexcept -> core::StringView;

    auto get_meta_bool(ResourceMetaView const& meta, core::stringid_arg_type key, bool& result) noexcept -> bool;

    auto get_meta_int32(ResourceMetaView const& meta, core::stringid_arg_type key, int32_t& result) noexcept -> bool;

    auto get_meta_float(ResourceMetaView const& meta, core::stringid_arg_type key, float& result) noexcept -> bool;

    auto get_meta_string(ResourceMetaView const& meta, core::stringid_arg_type key, core::StringView& result) noexcept -> bool;

} // namespace asset
