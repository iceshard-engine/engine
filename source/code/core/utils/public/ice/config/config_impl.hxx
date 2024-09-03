/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/config/config_types.hxx>

namespace ice::config::detail
{

    auto key(
        ice::Config const& config,
        ice::config::detail::ConfigKey const& key
    ) noexcept -> ice::String;

    auto find(
        ice::Config const& config,
        ice::u32 index,
        ice::config::detail::ConfigKey const*& out_key,
        ice::config::detail::ConfigValue const*& out_value
    ) noexcept -> ice::ErrorCode;

    auto find(
        ice::Config const& config,
        ice::String strkey,
        ice::config::detail::ConfigKey const*& out_key,
        ice::config::detail::ConfigValue const*& out_value
    ) noexcept -> ice::ErrorCode;

    template<typename T>
        requires (ice::concepts::ConfigValueType<T>)
    auto get(
        ice::Config const& config,
        ice::config::detail::ConfigKey const* key,
        ice::config::detail::ConfigValue const* value,
        T& out_value
    ) noexcept -> ice::ErrorCode;

    template<typename T, typename Key>
        requires (ice::concepts::ConfigValueType<T> && ice::concepts::ConfigKeyType<Key>)
    auto get(
        ice::Config const& config,
        Key key,
        T& out_value
    ) noexcept -> ice::ErrorCode
    {
        ice::config::detail::ConfigKey const* key_ptr = nullptr;
        ice::config::detail::ConfigValue const* val_ptr = nullptr;
        if (ErrorCode const err = ice::config::detail::find(config, key, key_ptr, val_ptr); err != S_Ok)
        {
            return err;
        }

        if (ErrorCode const err = ice::config::detail::get(config, key_ptr, val_ptr, out_value); err != S_Ok)
        {
            return err;
        }

        return S_Ok;
    }

} // namespace ice
