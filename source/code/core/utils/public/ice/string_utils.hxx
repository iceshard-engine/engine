/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/string/string.hxx>
#include <ice/string/heap_string.hxx>
#include <ice/log_formatters.hxx>
#include <ice/expected.hxx>
#include <ice/math.hxx>
#include <charconv>
#include <numeric>

namespace ice
{

    auto utf8_to_wide_size(ice::String path) noexcept -> ice::u32;
    bool utf8_to_wide_append(ice::String path, ice::HeapString<ice::wchar>& out_str) noexcept;
    auto utf8_to_wide(ice::Allocator& alloc, ice::String path) noexcept -> ice::HeapString<ice::wchar>;

    auto wide_to_utf8_size(ice::WString path) noexcept -> ice::u32;
    bool wide_to_utf8_append(ice::WString path, ice::HeapString<>& out_str) noexcept;
    auto wide_to_utf8(ice::Allocator& alloc, ice::WString path) noexcept -> ice::HeapString<>;

    template<typename StrType>
    struct FromCharsResult
    {
        ice::ErrorCode ec;
        StrType remaining;

        constexpr operator bool() const noexcept
        {
            return ec;
        }
    };

    template<typename T>
        requires (std::is_integral_v<T> || std::is_floating_point_v<T>)
    auto from_chars(ice::String str, T& out_value) noexcept -> ice::FromCharsResult<ice::String>
    {
        ice::ErrorCode res = ice::S_Ok;
        std::from_chars_result const fc_res = std::from_chars(
            ice::string::begin(str),
            ice::string::end(str),
            out_value
        );

        if (fc_res.ec == std::errc::result_out_of_range)
        {
            res = ice::E_OutOfRange;
        }
        else if (fc_res.ec == std::errc::invalid_argument)
        {
            res = ice::E_InvalidArgument;
        }

        return {
            .ec = res,
            .remaining = ice::String{ fc_res.ptr, ice::string::end(str) }
        };
    }

    template<typename T>
        requires (std::is_integral_v<T> || std::is_floating_point_v<T>)
    auto from_chars(char const* str_beg, char const* str_end, T& out_value) noexcept -> ice::FromCharsResult<char const*>
    {
        ice::ErrorCode res = ice::S_Ok;
        std::from_chars_result const fc_res = std::from_chars(
            str_beg,
            str_end,
            out_value
        );

        if (fc_res.ec == std::errc::result_out_of_range)
        {
            res = ice::E_OutOfRange;
        }
        else if (fc_res.ec == std::errc::invalid_argument)
        {
            res = ice::E_InvalidArgument;
        }

        return {
            .ec = res,
            .remaining = fc_res.ptr
        };
    }

    template<typename T>
        requires (std::is_integral_v<T> || std::is_floating_point_v<T>)
    auto from_chars(ice::String str, ice::String& out_str, T& out_value) noexcept -> ice::ErrorCode
    {
        ice::FromCharsResult<ice::String> const result = ice::from_chars(str, out_value);
        out_str = result.remaining;
        return result.ec;
    }

    inline auto from_chars(ice::String str, bool& out_value) noexcept -> ice::FromCharsResult<ice::String>
    {
        int temp_out = 0;
        ice::FromCharsResult<ice::String> const result = from_chars(str, temp_out);
        if (result.ec == ice::S_Ok)
        {
            out_value = bool(temp_out);
        }
        return result;
    }

    inline auto from_chars(ice::String str, ice::String& out_str, bool& out_value) noexcept -> ice::ErrorCode
    {
        ice::FromCharsResult<ice::String> const result = ice::from_chars(str, out_value);
        out_str = result.remaining;
        return result.ec;
    }

    namespace string
    {

        template<typename... Args>
        constexpr void push_format(
            ice::HeapString<char>& str,
            fmt::format_string<Args...> format,
            Args&&... args
        ) noexcept
        {
            ice::u32 const size = ice::u32(fmt::formatted_size(format, ice::forward<Args>(args)...));
            ice::u32 const new_size = ice::string::size(str) + size;
            if (new_size + 1 >= str._capacity)
            {
                ice::string::grow(str, new_size + 1);
            }
            fmt::format_to_n(ice::string::end(str), size, format, ice::forward<Args>(args)...);
            str._size += size;
            str._data[str._size] = '\0';
        }

    } // namespace string

} // namespace ice
