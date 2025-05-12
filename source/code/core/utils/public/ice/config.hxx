/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/container_types.hxx>
#include <ice/config/config_impl.hxx>

namespace ice::config
{

    auto from_data(ice::Data data) noexcept -> ice::Config;

    auto from_json(ice::Allocator& alloc, ice::String json, ice::Memory& out_memory) noexcept -> ice::Config;

    template<typename T, typename Key>
        requires (ice::concepts::ConfigValueType<T> && ice::concepts::ConfigKeyType<Key>)
    auto get(
        ice::Config const& config,
        Key key,
        T& out_value,
        ice::ConfigValueFlags flags = ConfigValueFlags::None
    ) noexcept -> ice::ErrorCode;

    template<typename T, typename Key>
        requires (ice::concepts::ConfigValueType<T> && ice::concepts::ConfigKeyType<Key>)
    auto get(
        ice::Config const& config,
        Key key,
        ice::ConfigValueFlags flags = ConfigValueFlags::None
    ) noexcept -> ice::Expected<T>;

    template<typename T, typename Key>
        requires (ice::concepts::ConfigValueType<T> && ice::concepts::ConfigKeyType<Key>)
    auto get_array(
        ice::Config const& config,
        Key key,
        ice::Array<T>& out_values,
        ice::ConfigValueFlags flags = ConfigValueFlags::None
    ) noexcept -> ice::ErrorCode;

    ////////////////////////////////////////////////////////////////////////////////////////////////

    template<typename T, typename Key>
        requires (ice::concepts::ConfigValueType<T> && ice::concepts::ConfigKeyType<Key>)
    auto get(
        ice::Config const& config,
        Key key,
        T& out_value,
        ice::ConfigValueFlags flags
    ) noexcept -> ice::ErrorCode
    {
        return ice::config::detail::get(config, key, out_value, flags);
    }

    template<typename T, typename Key>
        requires (ice::concepts::ConfigValueType<T> && ice::concepts::ConfigKeyType<Key>)
    auto get(
        ice::Config const& config,
        Key key,
        ice::ConfigValueFlags flags
    ) noexcept -> ice::Expected<T>
    {
        T result{};
        if (ErrorCode const err = ice::config::get(config, key, result, flags); err != S_Ok)
        {
            return err;
        }
        return result;
    }

    template<typename T, typename Key>
        requires (ice::concepts::ConfigValueType<T> && ice::concepts::ConfigKeyType<Key>)
    auto  get_array(
        ice::Config const& config,
        Key key,
        ice::Array<T>& out_values,
        ice::ConfigValueFlags flags
    ) noexcept -> ice::ErrorCode
    {
        return ice::config::detail::get_array(config, key, out_values, flags);
    }

} // namespace ice
