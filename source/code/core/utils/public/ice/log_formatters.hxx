#pragma once
#include <fmt/format.h>
#include <ice/stringid.hxx>
#include <ice/mem_types.hxx>

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
                return fmt::format_to(ctx.out(), "[sid:{:16x}]", ice::stringid_hash(value));
            }
            else
            {
                return fmt::format_to(ctx.out(), "[sid:{:16x}]'{}'", ice::stringid_hash(value), ice::stringid_hint(value));
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
