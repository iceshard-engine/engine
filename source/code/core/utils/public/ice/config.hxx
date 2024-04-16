/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_buffer.hxx>
#include <ice/mem_allocator.hxx>
#include <ice/container_types.hxx>
#include <ice/string/string.hxx>
#include <ice/expected.hxx>

namespace ice
{

    //! \brief A read-only interface for accessing config values.
    struct Config;

    //! \brief A read-write interface for accessing and updating config values.
    //!
    //! \note Can be loaded from a JSon file, but only alows saving to the internal binary representation.
    //! \note The object can be implicitly casted to the 'Config' interface.
    struct MutableConfig;


    //! \brief Deserializes given input data into a read-write config interface.
    //! \note The resulting config can be incomplete if input was invalid or incompatible.
    //!     If data might be invalid, please use `config_deserialize_into` for additional error handling and checking.
    [[nodiscard]]
    auto config_deserialize(ice::Allocator& alloc, ice::Data data) noexcept -> ice::MutableConfig;

    //! \brief Deserializes and appends all values from given input data to the read-write config interface.
    //! \return 'Res::Success' on success, otherwise returns first encountered error or last encountered warning.
    [[nodiscard]]
    auto config_deserialize_from(ice::MutableConfig& config, ice::Data data) noexcept -> ice::Expected<ice::ErrorCode>;

    //! \brief Reduces the Config buffers to the minimum required size to hold all values.
    //! \note This function can be helpfull to reduce the Config size before storing it in memory / files.
    //! \return The number of bytes saved due to internal buffer optimizations.
    //! \todo Not implemented.
    auto config_optimize(ice::MutableConfig& config) noexcept -> ice::usize = delete;

    //! \brief Tries to load config from given input.
    //! \warning The Config object is a view into the given 'Data' object.
    //!     The 'Config' filetime is directly tied to the 'Data' lifetime it was loaded from.
    //! \return Always returns a Config object. If data was invalid the object will be empty.
    [[nodiscard]]
    auto config_load(ice::Data data) noexcept -> ice::Config;

    //! \brief Stores the config object into the given memory block.
    //! \return Number of written bytes if successful, otherwise returns '0_B'.
    auto config_store(ice::Config const& config, ice::Memory out_data) noexcept -> ice::usize;

    //! \brief Saves the config object into a new memory block big enough to store all values.
    //! \note If config is empty it will store an empty config representation.
    //! \note If the allocator fails to provide a big enough memory block, the operation will fail.
    //! \return The newly allocated memory block with stored config if successful, otherwise a 'Null' memory block.
    [[nodiscard]]
    auto config_save(ice::Config const& config, ice::Allocator& alloc) noexcept -> ice::Memory;

    //! \return Memory requirements for the Config to be stored.
    [[nodiscard]]
    auto config_meminfo(ice::Config const& config) noexcept -> ice::meminfo;

    //! \brief The word value found at the start of a binary saved Config object.
    static constexpr ice::String Constant_FileHeader_ConfigFile = "ISCF";


    bool config_has_entry(
        ice::Config const& config,
        ice::StringID_Arg key
    ) noexcept;

    auto config_read_bool(
        ice::Config const& config,
        ice::StringID_Arg key,
        bool& result
    ) noexcept -> bool;

    auto config_read_int32(
        ice::Config const& config,
        ice::StringID_Arg key,
        ice::i32& result
    ) noexcept -> bool;

    auto config_read_float(
        ice::Config const& config,
        ice::StringID_Arg key,
        ice::f32& result
    ) noexcept -> bool;

    auto config_read_string(
        ice::Config const& config,
        ice::StringID_Arg key,
        ice::String& result
    ) noexcept -> bool;

    auto config_read_bool_array(
        ice::Config const& config,
        ice::StringID_Arg key,
        ice::Array<bool>& results
    ) noexcept -> bool;

    auto config_read_int32_array(
        ice::Config const& config,
        ice::StringID_Arg key,
        ice::Array<ice::i32>& results
    ) noexcept -> bool;

    auto config_read_float_array(
        ice::Config const& config,
        ice::StringID_Arg key,
        ice::Array<ice::f32>& results
    ) noexcept -> bool;

    auto config_read_string_array(
        ice::Config const& config,
        ice::StringID_Arg key,
        ice::Array<ice::String>& result
    ) noexcept -> bool;


    void config_set_bool(
        ice::MutableConfig& config,
        ice::StringID_Arg key,
        bool value
    ) noexcept;

    void config_set_int32(
        ice::MutableConfig& config,
        ice::StringID_Arg key,
        ice::i32 value
    ) noexcept;

    void config_set_float(
        ice::MutableConfig& config,
        ice::StringID_Arg key,
        ice::f32 value
    ) noexcept;

    void config_set_string(
        ice::MutableConfig& config,
        ice::StringID_Arg key,
        ice::String value
    ) noexcept;

    void config_set_bool_array(
        ice::MutableConfig& config,
        ice::StringID_Arg key,
        ice::Span<bool const> values
    ) noexcept;

    void config_set_int32_array(
        ice::MutableConfig& config,
        ice::StringID_Arg key,
        ice::Span<ice::i32 const> values
    ) noexcept;

    void config_set_float_array(
        ice::MutableConfig& config,
        ice::StringID_Arg key,
        ice::Span<ice::f32 const> values
    ) noexcept;

    void config_set_string_array(
        ice::MutableConfig& config,
        ice::StringID_Arg key,
        ice::Span<ice::String const> values
    ) noexcept;


    namespace detail
    {

        enum class ConfigEntryType : ice::u16
        {
            Invalid = 0x0,
            Boolean,
            Integer,
            Float,
            String,
            Buffer,
        };

        struct ConfigEntryBuffer
        {
            ice::u16 offset;
            ice::u16 size;
        };

        struct ConfigEntry
        {
            ConfigEntryType data_type;
            ice::u16 data_count;

            union
            {
                ice::i32 value_int;
                ice::f32 value_float;
                ConfigEntryBuffer value_buffer;
            };
        };

        static_assert(sizeof(ConfigEntry) == sizeof(ice::u64));

    } // namespace detail


    struct MutableConfig final
    {
        MutableConfig(ice::Allocator& alloc) noexcept;
        MutableConfig(MutableConfig&& other) noexcept;
        MutableConfig(MutableConfig const& other) noexcept = delete;
        ~MutableConfig() noexcept;

        auto operator=(MutableConfig&&) noexcept -> MutableConfig&;
        auto operator=(MutableConfig const&) noexcept -> MutableConfig& = delete;

        operator Config() const noexcept;

        ice::HashMap<detail::ConfigEntry> _config_entries;
        ice::Buffer _additional_data;
    };

    struct Config final
    {
        Config() noexcept;
        Config(Config&& other) noexcept;
        Config(Config const& other) noexcept;
        ~Config() noexcept = default;

        auto operator=(Config&& other) noexcept -> Config&;
        auto operator=(Config const& other) noexcept -> Config&;

        ice::HashMapView<detail::ConfigEntry> _config_entries;
        ice::Data _additional_data;
    };

} // namespace ice
