/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/config/config_types.hxx>
#include <ice/container/array.hxx>

namespace ice::config::detail
{

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

    auto gettype(
        ice::config::detail::ConfigKey const* key
    ) noexcept -> ice::ConfigValueType;

    bool istype(
        ice::config::detail::ConfigKey const* key,
        ice::ConfigValueType value_type
    ) noexcept;


    auto entry_key(
        ice::Config const& config,
        ice::config::detail::ConfigKey const& key
    ) noexcept -> ice::String;

    auto entry_first(
        ice::config::detail::ConfigKey const*& array_key,
        ice::config::detail::ConfigValue const*& array_value
    ) noexcept -> ice::ErrorCode;

    auto entry_next(
        ice::config::detail::ConfigKey const*& array_key,
        ice::config::detail::ConfigValue const*& array_value
    ) noexcept -> ice::ErrorCode;


    auto array_size(
        ice::config::detail::ConfigKey const* array_first_key
    ) noexcept -> ice::u32;

    auto array_first(
        ice::config::detail::ConfigKey const*& array_key,
        ice::config::detail::ConfigValue const*& array_value
    ) noexcept -> ice::ErrorCode;

    auto array_next(
        ice::config::detail::ConfigKey const*& array_key,
        ice::config::detail::ConfigValue const*& array_value
    ) noexcept -> ice::ErrorCode;


    template<typename T>
        requires (ice::concepts::ConfigValueType<T>)
    auto get(
        ice::Config const& config,
        ice::config::detail::ConfigKey const* key,
        ice::config::detail::ConfigValue const* value,
        T& out_value,
        ice::ConfigValueFlags flags
    ) noexcept -> ice::ErrorCode;

    template<typename T, typename Key>
        requires (ice::concepts::ConfigValueType<T> && ice::concepts::ConfigKeyType<Key>)
    auto get(
        ice::Config const& config,
        Key key,
        T& out_value,
        ice::ConfigValueFlags flags
    ) noexcept -> ice::ErrorCode
    {
        ice::config::detail::ConfigKey const* key_ptr = nullptr;
        ice::config::detail::ConfigValue const* val_ptr = nullptr;
        if (ErrorCode const err = ice::config::detail::find(config, key, key_ptr, val_ptr); err != S_Ok)
        {
            return err;
        }

        if (ErrorCode const err = ice::config::detail::get(config, key_ptr, val_ptr, out_value, flags); err != S_Ok)
        {
            return err;
        }

        return S_Ok;
    }

    template<typename T, typename Key>
        requires (ice::concepts::ConfigValueType<T> && ice::concepts::ConfigKeyType<Key>)
    auto get_array(
        ice::Config const& config,
        Key key,
        ice::Array<T>& out_values,
        ice::ConfigValueFlags flags
    ) noexcept -> ice::ErrorCode
    {
        ice::config::detail::ConfigKey const* key_ptr = nullptr;
        ice::config::detail::ConfigValue const* val_ptr = nullptr;
        if (ErrorCode const err = ice::config::detail::find(config, key, key_ptr, val_ptr); err != S_Ok)
        {
            return err;
        }

        if (ice::config::detail::istype(key_ptr, ConfigValueType::Table) == false)
        {
            return E_ConfigValueNotAnTable;
        }

        ice::config::detail::ConfigKey const* entry_key = key_ptr;
        ice::config::detail::ConfigValue const* entry_value = val_ptr;
        if (ErrorCode const err = array_first(entry_key, entry_value); err != S_Ok)
        {
            return err;
        }

        ice::u32 const table_size = array_size(entry_key);
        if (table_size == 0)
        {
            return S_Ok;
        }

        ice::ErrorCode result = S_Ok;
        out_values.reserve(table_size);
        do
        {
            T temp_value;
            if (result = ice::config::detail::get(config, entry_key, entry_value, temp_value, flags); result == S_Ok)
            {
                out_values.push_back(temp_value);
            }
            else if (result != E_ConfigValueTypeMissmatch)
            {
                break;
            }
        }
        while(array_next(entry_key, entry_value));

        return result;
    }

} // namespace ice
