/// Copyright 2024 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/types.hxx>
#include <ice/build/build.hxx>
#include <string_view>
#include <type_traits>
#include <cassert>

namespace ice
{

    //! \brief Compile-time defined error code with extended information.
    //!
    //! \details Error codes defined with this structure provide four different information values.
    //!   * \b type - Either `S` standing for \b Success or `E` standing for \b Error.
    //!   * \b code - Number value assigned to this error code. Value between `0` and `9999`. _(should be unique to ensure easy error identification)_
    //!   * \b category - User readable value that can be used to quickly indentify the source project of the error.
    //!   * \b description - User readable description of the error that can be logged to the output or other locations. 
    //! \details Although error codes should be mainly used to report errors and problems that resulted in expected behavior not being executed,
    //!   it is allowed to defined a \b Success code, and provide additional information if necessary. For example: _"Loading an asset succeeded but
    //!   because editor data was not parsable, debug settings were reset."_
    //! 
    //! \remark Currently error codes also follow some specific error ranges. 
    //!   * <b>[0000 - 2000)</b> - Core engine codes
    //!   * <b>[2000 - 4000)</b> - Framework codes
    //!   * <b>[4000 - 9999]</b> - Game codes
    struct ErrorCode
    {
        constexpr explicit ErrorCode(char const* definition) noexcept;

        //! \returns Character that defines the error code type. _(either `S` or `E`)_
        constexpr auto type() const noexcept -> char;

        //! \returns Parsed number value found in the definition string.
        constexpr auto code() const noexcept -> ice::i32;

        //! \returns Category of the error. May allow quicker identification of the actual issue.
        constexpr auto category() const noexcept -> std::string_view;
  
        //! \returns Description of the error.
        constexpr auto description() const noexcept -> std::string_view;

        //! \brief Implicit cast to a boolean value.
        //! \returns `true` if this error code is considered a `Success` value, `false` otherwise.
        constexpr operator bool() const noexcept { return type() != 'E'; }

        //! \brief Explicit conversion operator to `i32` value.
        constexpr explicit operator i32() const noexcept { return code(); }

        char const* value;
    };

    constexpr ErrorCode::ErrorCode(char const* definition) noexcept
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

    constexpr auto ErrorCode::type() const noexcept -> char
    {
        return value[0];
    }

    constexpr auto ErrorCode::code() const noexcept -> ice::i32
    {
        ice::i32 const code_base = type() == 'E' ? -1 : 1;
        ice::u32 constexpr code_offset = 2;
        return code_base * ((value[code_offset + 0] - '0') * 1000 + (value[code_offset + 1] - '0') * 100 + (value[code_offset + 2] - '0') * 10 + (value[code_offset + 3] - '0'));
    }

    constexpr auto ErrorCode::category() const noexcept -> std::string_view
    {
        char const* const start = value + 7;
        char const* end = start;
        while(*end != ':') ++end;
        return std::string_view{ start, size_t(end - start) };
    }

    constexpr auto ErrorCode::description() const noexcept -> std::string_view
    {
        char const* start = value + 7;
        while(*start != ':') ++start;
        return std::string_view{ start + 1 };
    }

    inline constexpr bool operator==(ErrorCode lhs, ErrorCode rhs) noexcept
    {
        return lhs.code() == rhs.code();
    }

    inline constexpr bool operator==(ErrorCode lhs, bool value) noexcept
    {
        return (bool)lhs == value;
    }

    // Special types to allow using S_Ok/S_Success and S_Fail/S_Error as general success/error checks.
    struct ErrorCodeSuccess : ErrorCode
    {
        constexpr explicit ErrorCodeSuccess(char const* definition) noexcept
            : ErrorCode{ definition }
        {
            assert(definition[0] == 'S');
        }
    };
    struct ErrorCodeError : ErrorCode
    {
        constexpr explicit ErrorCodeError(char const* definition) noexcept
            : ErrorCode{ definition }
        {
            assert(definition[0] == 'E');
        }
    };

    inline constexpr bool operator==(ErrorCodeSuccess lhs, ErrorCodeSuccess) noexcept
    {
        return true;
    }

    inline constexpr bool operator==(ErrorCode lhs, ErrorCodeSuccess) noexcept
    {
        return lhs.type() == 'S';
    }

    inline constexpr bool operator==(ErrorCodeError lhs, ErrorCodeError) noexcept
    {
        return true;
    }

    inline constexpr bool operator==(ErrorCode lhs, ErrorCodeError) noexcept
    {
        return lhs.type() == 'E';
    }

    inline constexpr bool operator==(ErrorCodeSuccess lhs, ErrorCodeError) noexcept
    {
        return false;
    }

} // namespace ice

#undef assert
