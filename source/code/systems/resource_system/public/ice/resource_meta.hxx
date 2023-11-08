/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/stringid.hxx>
#include <ice/string/string.hxx>
#include <ice/container_types.hxx>
#include <ice/resource_types.hxx>
#include <ice/mem_buffer.hxx>
#include <ice/result_codes.hxx>

namespace ice
{

    //! \brief A read-only interface for accessing resource metadata values.
    struct Metadata;

    //! \brief A read-write interface for accessing and updating resource metadata.
    //!
    //! \note Can be loaded from a JSon file, but only alows saving to the internal binary representation.
    //! \note The object can be implicitly casted to the 'Metadata' interface.
    struct MutableMetadata;


    //! \brief Deserializes given input data into a read-write metadata interface.
    //! \note The resulting metadata can be incomplete if input was invalid or incompatible.
    //!     If data might be invalid, please use `meta_deserialize_into` for additional error handling and checking.
    [[nodiscard]]
    auto meta_deserialize(ice::Allocator& alloc, ice::Data data) noexcept -> ice::MutableMetadata;

    //! \brief Deserializes and appends all values from given input data to the read-write metadata interface.
    //! \return 'Res::Success' on success, otherwise returns first encountered error or last encountered warning.
    [[nodiscard]]
    auto meta_deserialize_from(ice::MutableMetadata& meta, ice::Data data) noexcept -> ice::Result;

    //! \brief Reduces the Metadata buffers to the minimum required size to hold all values.
    //! \note This function can be helpfull to reduce the Metadata size before storing it in memory / files.
    //! \return The number of bytes saved due to internal buffer optimizations.
    //! \todo Not implemented.
    auto meta_optimize(ice::MutableMetadata& meta) noexcept -> ice::usize = delete;

    //! \brief Tries to load metadata from given input.
    //! \warning The Metadata object is a view into the given 'Data' object.
    //!     The 'Metadata' filetime is directly tied to the 'Data' lifetime it was loaded from.
    //! \return Always returns a Metadata object. If data was invalid the object will be empty.
    [[nodiscard]]
    auto meta_load(ice::Data data) noexcept -> ice::Metadata;

    //! \brief Stores the metadata object into the given memory block.
    //! \return Number of written bytes if successful, otherwise returns '0_B'.
    auto meta_store(ice::Metadata const& meta, ice::Memory out_data) noexcept -> ice::usize;

    //! \brief Saves the metadata object into a new memory block big enough to store all values.
    //! \note If metadata is empty it will store an empty metadata representation.
    //! \note If the allocator fails to provide a big enough memory block, the operation will fail.
    //! \return The newly allocated memory block with stored metadata if successful, otherwise a 'Null' memory block.
    [[nodiscard]]
    auto meta_save(ice::Metadata const& meta, ice::Allocator& alloc) noexcept -> ice::Memory;

    //! \return Memory requirements for the Metadata to be stored.
    [[nodiscard]]
    auto meta_meminfo(ice::Metadata const& meta) noexcept -> ice::meminfo;

    //! \brief The word value found at the start of a binary saved Metadata object.
    static constexpr ice::String Constant_FileHeader_MetadataFile = "ISMF";


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

} // namespace ice
