/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/config/config_details.hxx>

namespace ice
{

    enum class ConfigValueFlags : ice::u8
    {
        None,
        AllowImplicitCasts = 0x01,
        // AllowStringCasts = 0x02,
    };

    enum class ConfigValueType : ice::u8
    {
        Invalid,

        Bool,
        U8, U16, U32, U64,
        S8, S16, S32, S64,
        F32, F64,

        String,
        Blob, // Unused

        Table,
        Object,
    };

    class Config
    {
    public:
        Config() noexcept;
        Config(Config&& other) noexcept;
        Config(Config const& other) noexcept;

        auto operator=(Config&& other) noexcept -> Config&;
        auto operator=(Config const& other) noexcept -> Config&;

        auto operator[](ice::u32 index) const noexcept -> Config;
        auto operator[](ice::String key) const noexcept -> Config;

        ice::config::detail::ConfigKey const* _keys;
        ice::config::detail::ConfigValue const* _values;
        char const* _strings;
        void const* _data;
    };

} // namespace ice
