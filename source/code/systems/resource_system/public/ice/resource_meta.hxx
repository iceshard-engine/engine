/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/stringid.hxx>
#include <ice/string/string.hxx>
#include <ice/container_types.hxx>
#include <ice/resource_types.hxx>
#include <ice/mem_buffer.hxx>

namespace ice
{

    struct Metadata;

    struct MutableMetadata;


    void meta_deserialize(ice::Data data, ice::MutableMetadata& meta) noexcept;

    auto meta_load(ice::Data data) noexcept -> ice::Metadata;

    void meta_save(
        ice::Metadata const& meta,
        ice::Allocator& alloc,
        ice::Memory& out_data
    ) noexcept;

    bool meta_has_entry(
        ice::Metadata const& meta,
        ice::StringID_Arg key
    ) noexcept;

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
        ice::Array<bool>& results
    ) noexcept -> bool;

    auto meta_read_int32_array(
        ice::Metadata const& meta,
        ice::StringID_Arg key,
        ice::Array<ice::i32>& results
    ) noexcept -> bool;

    auto meta_read_flags_array(
        ice::Metadata const& meta,
        ice::StringID_Arg key,
        ice::Array<ice::ResourceFlags>& results
    ) noexcept -> bool;

    auto meta_read_float_array(
        ice::Metadata const& meta,
        ice::StringID_Arg key,
        ice::Array<ice::f32>& results
    ) noexcept -> bool;

    auto meta_read_string_array(
        ice::Metadata const& meta,
        ice::StringID_Arg key,
        ice::Array<ice::String, ContainerLogic::Complex>& result
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
            StringUTF8,
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

            union
            {
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
        ~MutableMetadata() noexcept;

        auto operator=(MutableMetadata&&) noexcept -> MutableMetadata&;
        auto operator=(MutableMetadata const&) noexcept -> MutableMetadata& = delete;

        operator Metadata() const noexcept;

        ice::HashMap<detail::MetadataEntry> _meta_entries;
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

        ice::HashMapView<detail::MetadataEntry> _meta_entries;
        ice::Data _additional_data;
    };


    static constexpr ice::String Constant_FileHeader_MetadataFile = "ISMF";
    static constexpr ice::String Constant_FileHeader_ResourceFile = "ISRF";

} // namespace ice
