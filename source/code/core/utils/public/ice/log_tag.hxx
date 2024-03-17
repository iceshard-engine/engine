/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/hash.hxx>
#include <ice/string/string.hxx>

namespace ice
{

    enum class LogTag : ice::u64
    {
        None = 0x0,

        Core = 0x1ull << 0,
        System = Core << 1,
        Module = Core << 2,
        Engine = Core << 3,
        Asset = Core << 4,
        Game = Core << 5,
        Tool = Core << 24,
    };

    struct LogTagDefinition
    {
        ice::LogTag const tag;
        ice::String const name;
    };

    constexpr auto create_log_tag(LogTag base_tag, ice::String name) noexcept -> LogTagDefinition
    {
        ice::u64 const name_hash = ice::hash32(name);
        ice::u64 const tag_hash = ice::hash(base_tag) << 32;
        return {
            .tag = static_cast<LogTag>(tag_hash | name_hash),
            .name = name
        };
    }

    constexpr auto create_log_tag(LogTagDefinition const& base_tag_def, ice::String name) noexcept -> LogTagDefinition
    {
        return create_log_tag(base_tag_def.tag, name);
    }

    void log_tag_register(ice::LogTagDefinition tag_def) noexcept;

    void log_tag_enable(ice::LogTag tag, bool enabled = true) noexcept;

    namespace detail
    {

        constexpr auto get_tag(ice::LogTag tag) noexcept
        {
            return tag;
        }

        constexpr auto get_tag(ice::LogTagDefinition const& tag_def) noexcept
        {
            return tag_def.tag;
        }

    } // namespace detail

} // namespace ice
