/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "config/config_detail.hxx"
#include <ice/string/var_string.hxx>

namespace ice::config::detail
{

    template<>
    auto config::detail::get(
        ice::Config const& config,
        ice::config::detail::ConfigKey const* key,
        ice::config::detail::ConfigValue const* value,
        ice::u32& out_value
    ) noexcept -> ice::ErrorCode
    {
        if (key == nullptr || key->vtype != ValType::CONFIG_VALTYPE_32B_BIT)
        {
            return E_ConfigValueTypeMissmatch;
        }

        out_value = value->internal;
        return S_Ok;
    }

    template<>
    auto config::detail::get(
        ice::Config const& config,
        ice::config::detail::ConfigKey const* key,
        ice::config::detail::ConfigValue const* value,
        ice::i32& out_value
    ) noexcept -> ice::ErrorCode
    {
        if (key == nullptr || key->vtype != (ValType::CONFIG_VALTYPE_32B_BIT | ValType::CONFIG_VALTYPE_SIGN_BIT))
        {
            return E_ConfigValueTypeMissmatch;
        }

        out_value = ice::bit_cast<ice::i32>(value->internal);
        return S_Ok;
    }

    template<>
    auto config::detail::get(
        ice::Config const& config,
        ice::config::detail::ConfigKey const* key,
        ice::config::detail::ConfigValue const* value,
        ice::String& out_value
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
