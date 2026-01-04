/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <fmt/format.h>
#include <ice/stringid.hxx>
#include <ice/mem_types.hxx>
#include <ice/string_types.hxx>
#include <ice/clock_types.hxx>
#include <ice/expected.hxx>

template<typename CharType>
struct fmt::formatter<ice::BasicString<CharType>> : public fmt::formatter<std::basic_string_view<CharType>>
{
    template<typename FormatContext>
    constexpr auto format(ice::BasicString<CharType> value, FormatContext& ctx) const noexcept
    {
        return fmt::formatter<std::basic_string_view<CharType>>::format(value, ctx);
    }
};

template<typename CharType>
struct fmt::formatter<ice::HeapString<CharType>> : public fmt::formatter<ice::BasicString<CharType>>
{
    template<typename FormatContext>
    constexpr auto format(ice::HeapString<CharType> const& value, FormatContext& ctx) const noexcept
    {
        return fmt::formatter<ice::BasicString<CharType>>::format({ value._data, value._size }, ctx);
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
    constexpr auto format(ice::StringID_Hash value, FormatContext& ctx) const noexcept
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
    constexpr auto format(ice::StringID_Arg value, FormatContext& ctx) const noexcept
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
struct fmt::formatter<ice::ncount>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    constexpr auto format(ice::ncount value, FormatContext& ctx) const noexcept
    {
        return fmt::format_to(ctx.out(), "{}", value.native());
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
    constexpr auto format(ice::usize value, FormatContext& ctx) const noexcept
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
struct fmt::formatter<ice::isize> : fmt::formatter<ice::usize>
{
    template<typename FormatContext>
    constexpr auto format(ice::isize value, FormatContext& ctx) const noexcept
    {
        auto output = ctx.out();
        if (value.value < 0)
        {
            output = fmt::format_to(ctx.out(), "-");
            value = -value;
        }
        return fmt::formatter<ice::usize>::format(value.to_usize(), ctx);
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
    constexpr auto format(ice::ErrorCode value, FormatContext& ctx) const noexcept
    {
        fmt::string_view const type = value.type() == 'E' ? "Error" : "Success";
        return fmt::format_to(ctx.out(), "{}({}, '{}')", type, value.category(), value.description());
    }
};

template<typename Val>
struct fmt::formatter<ice::Expected<Val, ice::ErrorCode>>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    constexpr auto format(ice::Expected<Val, ice::ErrorCode> const& value, FormatContext& ctx) const noexcept
    {
        if (value.succeeded())
        {
            return fmt::format_to(ctx.out(), "{}", ice::ErrorCode{ ice::S_Ok });
        }
        else
        {
            return fmt::format_to(ctx.out(), "{}", value.error());
        }
    }
};

template<ice::TimeType T>
struct fmt::formatter<T>
{
    static constexpr auto presentation_type(ice::Ts t) noexcept { return 's'; }
    static constexpr auto presentation_type(ice::Tms t) noexcept { return t.value > 9999 ? presentation_type(ice::Ts(t)) : 'm'; }
    static constexpr auto presentation_type(ice::Tus t) noexcept { return t.value > 9999 ? presentation_type(ice::Tms(t)) : 'u'; }
    static constexpr auto presentation_type(ice::Tns t) noexcept { return t.value > 9999 ? presentation_type(ice::Tus(t)) : 'n'; }

    // 'd' - dynamic, 's' - second, 'm' - millisecond, 'u' - microsecond, 'n' - nanosecond
    char presentation = 'd';// T::Constant_Precision <= ice::Tms::Constant_Precision ? 's' : 'u';
    ice::u8 floatingp = 0;

    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        auto it = ctx.begin(), end = ctx.end();

        // Parse the presentation format and store it in the formatter:
        if (it != end && it[0] == '.')
        {
            floatingp = it[1] - '0';
            it += 2;
        }

        if (it != end && (*it == 'd' || *it == 's' || *it == 'm' || *it == 'u' || *it == 'n')) presentation = *it++;


        // Check if reached the end of the range:
        if (it != end && *it != '}') throw fmt::format_error("invalid format");

        // Return an iterator past the end of the parsed range:
        return it;
    }

    template<typename FormatContext>
    constexpr auto format(ice::TimeType auto value, FormatContext& ctx) const noexcept
    {
        char final_presentation = presentation;
        if (final_presentation == 'd')
        {
            final_presentation = presentation_type(value);
        }

        if (floatingp == 0)
        {
            switch(final_presentation)
            {
            case 's': return fmt::format_to(ctx.out(), "{:.3f}s", ice::Ts(value).value, floatingp);
            case 'm': return fmt::format_to(ctx.out(), "{}ms", ice::Tms(value).value);
            case 'u': return fmt::format_to(ctx.out(), "{}us", ice::Tus(value).value);
            case 'n':
            default: return fmt::format_to(ctx.out(), "{}ns", ice::Tns(value).value);
            }
        }
        else
        {
            switch(final_presentation)
            {
            case 's': return fmt::format_to(ctx.out(), "{:.{}f}s", ice::Ts(value).value, floatingp);
            case 'm': return fmt::format_to(ctx.out(), "{:.{}f}ms", ice::Tms(value).value * 1.0, floatingp);
            case 'u': return fmt::format_to(ctx.out(), "{:.{}f}us", ice::Tus(value).value * 1.0, floatingp);
            case 'n':
            default: return fmt::format_to(ctx.out(), "{:.{}f}ns", ice::Tns(value).value * 1.0, floatingp);
            }
        }
    }
};
