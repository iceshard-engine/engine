/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/config/config_impl.hxx>

namespace ice::config
{

    auto from_data(ice::Data data) noexcept -> ice::Config;

    auto from_json(ice::Allocator& alloc, ice::String json, ice::Memory& out_memory) noexcept -> ice::Config;

    template<typename T, typename Key>
        requires (ice::concepts::ConfigValueType<T> && ice::concepts::ConfigKeyType<Key>)
    auto get(ice::Config const& config, Key key, T& out_value) noexcept -> ice::ErrorCode;

    template<typename T, typename Key>
        requires (ice::concepts::ConfigValueType<T> && ice::concepts::ConfigKeyType<Key>)
    auto get(ice::Config const& config, Key key) noexcept -> ice::Expected<T>;

    ////////////////////////////////////////////////////////////////////////////////////////////////

    template<typename T, typename Key>
        requires (ice::concepts::ConfigValueType<T> && ice::concepts::ConfigKeyType<Key>)
    auto get(ice::Config const& config, Key key, T& out_value) noexcept -> ice::ErrorCode
    {
        return ice::config::detail::get(config, key, out_value);
    }

    template<typename T, typename Key>
        requires (ice::concepts::ConfigValueType<T> && ice::concepts::ConfigKeyType<Key>)
    auto get(ice::Config const& config, Key key) noexcept -> ice::Expected<T>
    {
        T result{};
        if (ErrorCode const err = ice::config::get(config, key, result); err != S_Ok)
        {
            return err;
        }
        return result;
    }

} // namespace ice
