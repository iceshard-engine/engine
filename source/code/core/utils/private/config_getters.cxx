/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "config/config_detail.hxx"
#include <ice/string/var_string.hxx>

namespace ice::config::detail
{

    template<typename T> requires (std::is_same_v<T, ice::String> == false)
    auto try_get_casted(
        ice::Config const& config,
        ice::config::detail::ConfigKey const* key,
        ice::config::detail::ConfigValue const* value,
        T& out_value,
        ice::ConfigValueFlags flags
    ) noexcept -> ice::ErrorCode
    {
        if (key == nullptr
            || key->vtype == CONFIG_VALTYPE_NONE
            || key->vtype >= CONFIG_VALTYPE_OTHER
            || ice::has_none(flags, ConfigValueFlags::AllowImplicitCasts))
        {
            return E_ConfigValueTypeMissmatch;
        }

        ice::u64 generic_value = value->internal;

        ValType const valtype = static_cast<ValType>(key->vtype);
        bool const is64bit = ice::has_any(valtype, CONFIG_VALTYPE_64B_BIT);
        if (is64bit)
        {
            // TODO: Is unaligned access worse than performing 32bit moves?
            ice::u32 const* bitdata = reinterpret_cast<ice::u32 const*>(ice::ptr_add(config._data, {value->internal}));
            generic_value = bitdata[1];
            generic_value <<= 32;
            generic_value += bitdata[0];
        }

        if (ice::has_any(valtype, CONFIG_VALTYPE_FP_BIT))
        {
            if constexpr (std::is_floating_point_v<T>)
            {
                out_value = static_cast<T>(
                    is64bit
                    ? std::bit_cast<ice::f64>(generic_value)
                    : std::bit_cast<ice::f32>(ice::u32(generic_value))
                );
            }
            else
            {
                out_value = static_cast<T>(
                    static_cast<ice::i64>( // Need to cast to i64 before casting down to any unsigned value
                        is64bit
                        ? std::bit_cast<ice::f64>(generic_value)
                        : std::bit_cast<ice::f32>(ice::u32(generic_value))
                    )
                );
            }
        }
        else if (ice::has_any(valtype, CONFIG_VALTYPE_SIGN_BIT))
        {
            ice::i64 valsigned;
            if (ice::has_any(valtype, CONFIG_VALTYPE_8B_BIT))
            {
                valsigned = std::bit_cast<ice::i8>(ice::u8(generic_value));
            }
            else if (ice::has_any(valtype, CONFIG_VALTYPE_16B_BIT))
            {
                valsigned = std::bit_cast<ice::i16>(ice::u16(generic_value));
            }
            else if (ice::has_any(valtype, CONFIG_VALTYPE_32B_BIT))
            {
                valsigned = std::bit_cast<ice::i32>(ice::u32(generic_value));
            }
            else
            {
                valsigned = std::bit_cast<ice::i64>(generic_value);
            }

            if constexpr (std::is_unsigned_v<T>)
            {
                if (valsigned < 0)
                {
                    valsigned = 0;
                }
            }

            out_value = static_cast<T>(valsigned);
        }
        else
        {
            out_value = static_cast<T>(generic_value);
        }
        return S_Ok;
    }

    template<>
    auto config::detail::get(
        ice::Config const& config,
        ice::config::detail::ConfigKey const* key,
        ice::config::detail::ConfigValue const* value,
        bool& out_value,
        ice::ConfigValueFlags flags
    ) noexcept -> ice::ErrorCode
    {
        if (key == nullptr || key->vtype != ValType::CONFIG_VALTYPE_SIGN_BIT)
        {
            return E_ConfigValueTypeMissmatch;
        }

        out_value = static_cast<bool>(value->internal);
        return S_Ok;
    }

    template<>
    auto config::detail::get(
        ice::Config const& config,
        ice::config::detail::ConfigKey const* key,
        ice::config::detail::ConfigValue const* value,
        ice::u8& out_value,
        ice::ConfigValueFlags flags
    ) noexcept -> ice::ErrorCode
    {
        if (key == nullptr || key->vtype != ValType::CONFIG_VALTYPE_8B_BIT)
        {
            return try_get_casted(config, key, value, out_value, flags);
        }

        out_value = static_cast<ice::u8>(value->internal);
        return S_Ok;
    }

    template<>
    auto config::detail::get(
        ice::Config const& config,
        ice::config::detail::ConfigKey const* key,
        ice::config::detail::ConfigValue const* value,
        ice::u32& out_value,
        ice::ConfigValueFlags flags
    ) noexcept -> ice::ErrorCode
    {
        if (key == nullptr || key->vtype != ValType::CONFIG_VALTYPE_32B_BIT)
        {
            return try_get_casted(config, key, value, out_value, flags);
        }

        out_value = value->internal;
        return S_Ok;
    }

    template<>
    auto config::detail::get(
        ice::Config const& config,
        ice::config::detail::ConfigKey const* key,
        ice::config::detail::ConfigValue const* value,
        ice::i32& out_value,
        ice::ConfigValueFlags flags
    ) noexcept -> ice::ErrorCode
    {
        if (key == nullptr || key->vtype != (ValType::CONFIG_VALTYPE_32B_BIT | ValType::CONFIG_VALTYPE_SIGN_BIT))
        {
            return try_get_casted(config, key, value, out_value, flags);
        }

        out_value = ice::bit_cast<ice::i32>(value->internal);
        return S_Ok;
    }

    template<>
    auto config::detail::get(
        ice::Config const& config,
        ice::config::detail::ConfigKey const* key,
        ice::config::detail::ConfigValue const* value,
        ice::f32& out_value,
        ice::ConfigValueFlags flags
    ) noexcept -> ice::ErrorCode
    {
        if (key == nullptr || key->vtype != (ValType::CONFIG_VALTYPE_32B_BIT | ValType::CONFIG_VALTYPE_SIGN_BIT | ValType::CONFIG_VALTYPE_FP_BIT))
        {
            return try_get_casted(config, key, value, out_value, flags);
        }

        out_value = ice::bit_cast<ice::f32>(value->internal);
        return S_Ok;
    }

    template<>
    auto config::detail::get(
        ice::Config const& config,
        ice::config::detail::ConfigKey const* key,
        ice::config::detail::ConfigValue const* value,
        ice::String& out_value,
        ice::ConfigValueFlags flags
    ) noexcept -> ice::ErrorCode
    {
        if (key == nullptr || key->vtype != ValType::CONFIG_VALTYPE_STRING)
        {
            return E_ConfigValueTypeMissmatch;
        }

        char const* strdata = reinterpret_cast<char const*>(config._data) + value->internal;
        ice::ucount varbytes = 0;
        ice::ucount const size = ice::string::detail::read_varstring_size(strdata, varbytes);

        out_value = { strdata + varbytes, size };
        return S_Ok;
    }

} // namespace ice
