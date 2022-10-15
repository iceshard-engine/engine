#pragma once
#include <fmt/format.h>
#include <ice/stringid.hxx>
#include <ice/mem_types.hxx>

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
