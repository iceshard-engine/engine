/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <fmt/format.h>
#include <ice/stringid.hxx>
#include <ice/mem_types.hxx>
#include <ice/string_types.hxx>

template<typename CharType>
struct fmt::formatter<ice::BasicString<CharType>> : public fmt::formatter<std::basic_string_view<CharType>>
{
    template<typename FormatContext>
    constexpr auto format(ice::BasicString<CharType> value, FormatContext& ctx)
    {
        return fmt::formatter<std::basic_string_view<CharType>>::format({ value._data, value._size }, ctx);
    }
};

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
    char presentation = 'b'; // i - integral, b - bytes, p - pretty, P - pretty with spaces

    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        auto it = ctx.begin(), end = ctx.end();
        // Parse the presentation format and store it in the formatter:
        if (it != end && (*it == 'i' || *it == 'b' || *it == 'p' || *it == 'P')) presentation = *it++;

        // Check if reached the end of the range:
        if (it != end && *it != '}') throw fmt::format_error("invalid format");

        // Return an iterator past the end of the parsed range:
        return it;
    }

    template<typename FormatContext>
    constexpr auto format(ice::usize value, FormatContext& ctx)
    {
        using namespace ice;

        switch (presentation)
        {
        case 'i':
            return fmt::format_to(ctx.out(), "{}", value.value);
        case 'p':
        case 'P':
        {
            if (value < 1_KiB)
            {
                if (presentation == 'P')
                {
                    return fmt::format_to(ctx.out(), "{} B", value.value);
                }
                else
                {
                    return fmt::format_to(ctx.out(), "{}B", value.value);
                }
            }
            else if (value < 1_MiB)
            {
                if (presentation == 'P')
                {
                    return fmt::format_to(ctx.out(), "{} KiB {} B", (value.value / 1024), value.value % 1024);
                }
                else
                {
                    return fmt::format_to(ctx.out(), "{}KiB {}B", (value.value / 1024), value.value % 1024);
                }
            }
            else
            {
                if (presentation == 'P')
                {
                    return fmt::format_to(
                        ctx.out(),
                        "{} MiB {} KiB {} B",
                        (value.value / (1024 * 1024)), (value.value / 1024) % 1024, value.value % 1024
                    );
                }
                else
                {
                    return fmt::format_to(
                        ctx.out(),
                        "{}MiB {}KiB {}B",
                        (value.value / (1024 * 1024)), (value.value / 1024) % 1024, value.value % 1024
                    );
                }
            }
        }
        default:
            return fmt::format_to(ctx.out(), "{}B", value.value);
        }
    }
};

template<>
struct fmt::formatter<ice::ErrorCode>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    constexpr auto format(ice::ErrorCode value, FormatContext& ctx)
    {
        fmt::string_view const type = value.type() == 'E' ? "Error" : "Success";
        return fmt::format_to(ctx.out(), "{}({}, '{}')", type, value.category(), value.description());
    }
};
