#pragma once
#include "config_internal.hxx"

namespace ice::config::detail
{

    auto get_keyindex(ice::String key) noexcept -> ice::u32;

    auto get_subconfig(ice::Config& config, ice::String key) noexcept -> ice::ErrorCode;

    auto get_subconfig(ice::Config& config, ice::u32 idx) noexcept -> ice::ErrorCode;

    auto keyval(
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
        ice::String key,
        ice::config::detail::ConfigKey const*& out_key,
        ice::config::detail::ConfigValue const*& out_value
    ) noexcept -> ice::ErrorCode;

} // namespace ice::config::detail
