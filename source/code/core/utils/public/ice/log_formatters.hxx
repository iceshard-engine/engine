/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <fmt/format.h>
#include <ice/stringid.hxx>
#include <ice/mem_types.hxx>
#include <ice/result_codes.hxx>

template<>
struct fmt::formatter<ice::StringID_Hash>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    constexpr auto format(ice::StringID_Hash value, FormatContext& ctx)
    {
        if (value.value == ice::stringid_hash(ice::StringID_Invalid).value)
        {
            return fmt::format_to(ctx.out(), "[sid_hash:<invalid>]");
        }
        else
        {
            return fmt::format_to(ctx.out(), "[sid_hash:{:16x}]", value.value);
        }
    }
};

template<bool DebugImpl>
struct fmt::formatter<ice::BaseStringID<DebugImpl>>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    constexpr auto format(ice::StringID_Arg value, FormatContext& ctx)
    {
        if (value == ice::StringID_Invalid)
        {
            return fmt::format_to(ctx.out(), "[sid:<invalid>]");
        }
        else
        {
            if constexpr (DebugImpl == false)
            {
                return fmt::format_to(ctx.out(), "[sid:{:16x}]", ice::stringid_hash(value).value);
            }
            else
            {
                return fmt::format_to(ctx.out(), "[sid:{:16x}]'{}'", ice::stringid_hash(value).value, ice::stringid_hint(value));
            }
        }
    }
};

template<>
struct fmt::formatter<ice::usize>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    constexpr auto format(ice::usize value, FormatContext& ctx)
    {
        return fmt::format_to(ctx.out(), "{}B", value.value);
    }
};

template<bool DebugInfo>
struct fmt::formatter<ice::ResultCode<DebugInfo>>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    static constexpr std::string_view SeverityVal[]{
        "OK", // Success
        "INF",
        "WAR",
        "ERR"
    };

    template<typename FormatContext>
    constexpr auto format(ice::ResultCode<DebugInfo> value, FormatContext& ctx)
    {
        if (value == ice::Res::Success)
        {
            return fmt::format_to(ctx.out(), "OK");
        }

        if constexpr (DebugInfo == true)
        {
            return fmt::format_to(ctx.out(), "{}({} '{}')", SeverityVal[value.value.severity], value.value.id, value.description);
        }
        else
        {
            return fmt::format_to(ctx.out(), "{}({})", SeverityVal[value.value.severity], value.value.id);
        }
    }
};
