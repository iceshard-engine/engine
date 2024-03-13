/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/types.hxx>
#include <ice/build/build.hxx>
#include <string_view>
#include <type_traits>
#include <cassert>

namespace ice
{

    struct ErrorCode
    {
        constexpr explicit ErrorCode(char const* definition) noexcept
            : value{ definition }
        {
#if ICE_DEBUG || ICE_DEVELOP
            assert(std::is_constant_evaluated());
            assert(value[1] == '.');
            assert(value[2] >= '0' && value[2] <= '9');
            assert(value[3] >= '0' && value[3] <= '9');
            assert(value[4] >= '0' && value[4] <= '9');
            assert(value[5] >= '0' && value[5] <= '9');
            assert(value[6] == ':');
            char const* category = value + 7;
            while(*category != ':' && *category != '\0') category += 1;
            assert(*category == ':');

            assert(type() != 'E' || code() != 0);
#endif
        }

        constexpr auto type() const noexcept -> char
        {
            return value[0];
        }

        constexpr auto code() const noexcept -> ice::i32
        {
            ice::i32 const code_base = type() == 'E' ? -1 : 1;
            ice::u32 constexpr code_offset = 2;
            return code_base * ((value[code_offset + 0] - '0') * 1000 + (value[code_offset + 1] - '0') * 100 + (value[code_offset + 2] - '0') * 10 + (value[code_offset + 3] - '0'));
        }

        constexpr auto category() const noexcept -> std::string_view
        {
            char const* const start = value + 7;
            char const* end = start;
            while(*end != ':') ++end;
            return std::string_view{ start, size_t(end - start) };
        }

        constexpr auto description() const noexcept -> std::string_view
        {
            char const* start = value + 7;
            while(*start != ':') ++start;
            return std::string_view{ start + 1 };
        }

        constexpr operator bool() const noexcept { return type() != 'E'; }
        constexpr explicit operator i32() const noexcept { return code(); }

        char const* value;
    };

    inline constexpr bool operator==(ErrorCode lhs, ErrorCode rhs) noexcept
    {
        return lhs.code() == rhs.code();
    }

    inline constexpr bool operator==(ErrorCode lhs, bool value) noexcept
    {
        return (bool)lhs == value;
    }

} // namespace ice

#undef assert
