/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/params_types.hxx>
#include <ice/container_types.hxx>

namespace ice
{

    //! \brief Additional flags altering command line parameter behaviors.
    enum class ParamFlags : ice::u32
    {
        None = 0x00'00,
        IsRequired = 0x00'01,

        AllowExtraArgs = 0x00'02,
        NoExtraArgs = 0x00'04,

        TakeFirst = 0x01'00,
        TakeLast = 0x02'00,
        TakeAll = 0x04'00,

        ValidateFile = 0x10'00,
        ValidateDirectory = 0x20'00,
        ValidatePath = ValidateFile | ValidateDirectory,

        All = IsRequired
            | AllowExtraArgs | NoExtraArgs
            | TakeFirst | TakeLast | TakeAll
            | ValidateFile | ValidateDirectory
    };

    //! \brief Basic information for each command line parameter.
    struct ParamDefinition
    {
        //! \brief Name(s) for the parameter to be accessible through.
        //! \details Allowed names are single dashes, double dashes and positional names, ex.: "-s,--multiple,positional"
        //! \note Names need to be unique across all defined parameters.
        ice::String name;

        //! \brief The description shown in the '--help' output.
        ice::String description = {};

        //! \brief A group this parameter will be shown part in the '--help' output.
        ice::String group = {};

        //! \brief Special name for the parameter accepted type hint. Defaults to parameter native type, ex.: 'ice::i32' -> 'INT'
        ice::String type_name = {};

        //! \brief Minimum number of arguments required to be considered valid usage.
        //! \note Only usable by Array and Custom parameter definitions.
        ice::i32 min = 0;

        //! \brief Maximum number of arguments required to be considered valid usage.
        //! \note Only usable by Array and Custom parameter definitions.
        ice::i32 max = ice::i32_max;

        //! \brief The number of arguments expected fot this type size.
        //! \note Only usable by Array and Custom parameter definitions.
        ice::arr2i typesize = { 0, ice::i32_max };

        //! \brief Flags additionally altering parsing behavior of the parameter.
        ice::ParamFlags flags = ice::ParamFlags::None;
    };

    [[nodiscard]]
    auto create_params(
        ice::Allocator& alloc,
        ice::String name,
        ice::String version,
        ice::String description
    ) noexcept -> ice::Params;

    [[nodiscard]]
    auto params_process(ice::Params& params, int argc, char const* const* argv) noexcept -> ice::i32;

    template<typename T>
    bool params_define(ice::Params& params, ice::ParamDefinition const& definition, T& out_value) noexcept = delete;

    template<> bool params_define(ice::Params& params, ice::ParamDefinition const& definition, bool& out_value) noexcept;
    template<> bool params_define(ice::Params& params, ice::ParamDefinition const& definition, char& out_value) noexcept;
    template<> bool params_define(ice::Params& params, ice::ParamDefinition const& definition, ice::f32& out_value) noexcept;
    template<> bool params_define(ice::Params& params, ice::ParamDefinition const& definition, ice::f64& out_value) noexcept;
    template<> bool params_define(ice::Params& params, ice::ParamDefinition const& definition, ice::u8& out_value) noexcept;
    template<> bool params_define(ice::Params& params, ice::ParamDefinition const& definition, ice::u16& out_value) noexcept;
    template<> bool params_define(ice::Params& params, ice::ParamDefinition const& definition, ice::u32& out_value) noexcept;
    template<> bool params_define(ice::Params& params, ice::ParamDefinition const& definition, ice::u64& out_value) noexcept;
    template<> bool params_define(ice::Params& params, ice::ParamDefinition const& definition, ice::i8& out_value) noexcept;
    template<> bool params_define(ice::Params& params, ice::ParamDefinition const& definition, ice::i16& out_value) noexcept;
    template<> bool params_define(ice::Params& params, ice::ParamDefinition const& definition, ice::i32& out_value) noexcept;
    template<> bool params_define(ice::Params& params, ice::ParamDefinition const& definition, ice::i64& out_value) noexcept;
    template<> bool params_define(ice::Params& params, ice::ParamDefinition const& definition, ice::String& out_value) noexcept;
    template<> bool params_define(ice::Params& params, ice::ParamDefinition const& definition, ice::HeapString<>& out_value) noexcept;

    template<> bool params_define(ice::Params& params, ice::ParamDefinition const& definition, ice::Array<ice::String>& out_values) noexcept;
    template<> bool params_define(ice::Params& params, ice::ParamDefinition const& definition, ice::Array<ice::HeapString<>>& out_values) noexcept;

    bool params_define_custom(
        ice::Params& params,
        ice::ParamDefinition const& definition,
        void* userdata,
        ice::ParamsCustomCallback callback
    ) noexcept;

    void params_register_globals(ice::Params& params) noexcept;


    //! \brief Base class for global defined parameter instances.
    struct ParamInstanceBase
    {
        ice::ParamInstanceBase* const _next;
        ice::ParamDefinition const definition;

        ParamInstanceBase(ice::String category, ice::String name, ice::String description = {}) noexcept;
        ParamInstanceBase(ice::ParamDefinition const& definition) noexcept;

        virtual bool on_register(ice::Params& params) noexcept = 0;
    };

    //! \brief Class for global defined parameter instances of simple native types.
    template<typename T> requires(std::is_trivial_v<T>)
    class ParamInstance final : public ice::ParamInstanceBase
    {
    public:
        using ParamInstanceBase::ParamInstanceBase;

        bool on_register(ice::Params& params) noexcept override
        {
            return params_define(params, definition, value);
        }

        inline operator T() const noexcept { return value; }

        T value{};
    };

    //! \brief Class for global defined parameter instances of custom parameter types.
    template<typename T> requires ice::concepts::ParamCustomType<T>
    class ParamInstanceCustom final : public ice::ParamInstanceBase
    {
    public:
        using ParamInstanceBase::ParamInstanceBase;

        static bool param_parse_results_wrapper(void* ud, ice::Span<ice::String const> results) noexcept
        {
            return T::param_parse_results(*reinterpret_cast<T*>(ud), results);
        }

        bool on_register(ice::Params& params) noexcept override
        {
            return params_define_custom(params, definition, &value, &param_parse_results_wrapper);
        }

        inline operator T() const noexcept { return value; }

        T value{};
    };

} // namespace ice
