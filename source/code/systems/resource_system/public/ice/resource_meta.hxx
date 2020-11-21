#pragma once
#include <ice/string.hxx>
#include <ice/stringid.hxx>
#include <ice/buffer.hxx>
#include <ice/pod/array.hxx>

namespace ice
{

    struct Metadata;

    struct MutableMetadata;


    void meta_deserialize(ice::Data data, ice::MutableMetadata& meta) noexcept;

    auto meta_load(ice::Data data) noexcept -> ice::Metadata;

    void meta_store(ice::Metadata const& meta, ice::Buffer& buffer) noexcept;


    auto meta_read_bool(
        ice::Metadata const& meta,
        ice::StringID_Arg key,
        bool& result
    ) noexcept -> bool;

    auto meta_read_int32(
        ice::Metadata const& meta,
        ice::StringID_Arg key,
        ice::i32& result
    ) noexcept -> bool;

    auto meta_read_float(
        ice::Metadata const& meta,
        ice::StringID_Arg key,
        ice::f32& result
    ) noexcept -> bool;

    auto meta_read_string(
        ice::Metadata const& meta,
        ice::StringID_Arg key,
        ice::String& result
    ) noexcept -> bool;

    auto meta_read_bool_array(
        ice::Metadata const& meta,
        ice::StringID_Arg key,
        ice::pod::Array<bool>& results
    ) noexcept -> bool;

    auto meta_read_int32_array(
        ice::Metadata const& meta,
        ice::StringID_Arg key,
        ice::pod::Array<ice::i32>& results
    ) noexcept -> bool;

    auto meta_read_float_array(
        ice::Metadata const& meta,
        ice::StringID_Arg key,
        ice::pod::Array<ice::f32>& results
    ) noexcept -> bool;

    auto meta_read_string_array(
        ice::Metadata const& meta,
        ice::StringID_Arg key,
        ice::pod::Array<ice::String>& result
    ) noexcept -> bool;


    void meta_set_bool(
        ice::MutableMetadata& meta,
        ice::StringID_Arg key,
        bool value
    ) noexcept;

    void meta_set_int32(
        ice::MutableMetadata& meta,
        ice::StringID_Arg key,
        ice::i32 value
    ) noexcept;

    void meta_set_float(
        ice::MutableMetadata& meta,
        ice::StringID_Arg key,
        ice::f32 value
    ) noexcept;

    void meta_set_string(
        ice::MutableMetadata& meta,
        ice::StringID_Arg key,
        ice::String value
    ) noexcept;

    void meta_set_bool_array(
        ice::MutableMetadata& meta,
        ice::StringID_Arg key,
        ice::Span<bool const> values
    ) noexcept;

    void meta_set_int32_array(
        ice::MutableMetadata& meta,
        ice::StringID_Arg key,
        ice::Span<ice::i32 const> values
    ) noexcept;

    void meta_set_float_array(
        ice::MutableMetadata& meta,
        ice::StringID_Arg key,
        ice::Span<ice::f32 const> values
    ) noexcept;

    void meta_set_string_array(
        ice::MutableMetadata& meta,
        ice::StringID_Arg key,
        ice::Span<ice::String const> values
    ) noexcept;


    namespace detail
    {

        enum class MetadataEntryType : ice::u16
        {
            Invalid = 0x0,
            Boolean,
            Integer,
            Float,
            String,
            Buffer,
        };

        struct MetadataEntryBuffer
        {
            ice::u16 offset;
            ice::u16 size;
        };

        struct MetadataEntry
        {
            MetadataEntryType data_type;
            ice::u16 data_count;

            union {
                ice::i32 value_int;
                ice::f32 value_float;
                MetadataEntryBuffer value_buffer;
            };
        };

        static_assert(sizeof(MetadataEntry) == sizeof(ice::u64));

    } // namespace detail


    struct MutableMetadata final
    {
        MutableMetadata(ice::Allocator& alloc) noexcept;
        MutableMetadata(MutableMetadata&& other) noexcept;
        MutableMetadata(MutableMetadata const& other) noexcept = delete;
        ~MutableMetadata() noexcept = default;

        auto operator=(MutableMetadata&&) noexcept -> MutableMetadata&;
        auto operator=(MutableMetadata const&) noexcept -> MutableMetadata& = delete;

        operator Metadata() const noexcept;

        ice::pod::Hash<detail::MetadataEntry> _meta_entries;
        ice::Buffer _additional_data;
    };

    struct Metadata final
    {
        Metadata() noexcept;
        Metadata(Metadata&& other) noexcept;
        Metadata(Metadata const& other) noexcept;
        ~Metadata() noexcept = default;

        auto operator=(Metadata&& other) noexcept -> Metadata&;
        auto operator=(Metadata const& other) noexcept -> Metadata&;

        ice::pod::Hash<detail::MetadataEntry> _meta_entries;
        ice::Data _additional_data;
    };

} // namespace ice
