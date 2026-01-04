/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/string.hxx>
#include <ice/heap_string.hxx>
#include <ice/log_formatters.hxx>
#include <ice/expected.hxx>
#include <ice/math.hxx>
#include <charconv>
#include <numeric>

namespace ice
{

    namespace string
    {

        template<typename... Args>
        constexpr void push_format(
            ice::string::ResizableStringType auto& str,
            fmt::format_string<Args...> format,
            Args&&... args
        ) noexcept;

        template<ice::u32 Capacity, typename... Args>
        constexpr void push_format(
            ice::StaticString<Capacity, char>& str,
            fmt::format_string<Args...> format,
            Args&&... args
        ) noexcept;

        template<typename Fn>
        constexpr auto for_each_split(
            ice::String contents,
            ice::String separators,
             Fn&& fn
        ) noexcept -> ice::u32;

    } // namespace string

    enum class CaseSensitive : bool { No, Yes };
    enum class CompareResult : ice::i8 { Smaller = -1, Equal = 0, Larger = 1 };
    auto compare(ice::String left, ice::String right, ice::CaseSensitive = CaseSensitive::No) noexcept -> ice::CompareResult;
    auto compare(ice::String left, ice::String right, ice::u64 count, ice::CaseSensitive = CaseSensitive::No) noexcept -> ice::CompareResult;

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
        std::from_chars_result fc_res;
        if constexpr (std::is_integral_v<T>)
        {
            fc_res = std::from_chars(
                str.begin(),
                str.end(),
                out_value
            );
        }
        else
        {
#if ISP_COMPILER_CLANG <= 20 || ISP_WEBAPP || ISP_ANDROID
            // Because Libc++ did not support from_chars for floats up until clang.20 we need to use the old C style approach...
            // We don't try to handle errors in this version.
            fc_res.ec = std::errc{};
            char* ptr_end = nullptr; // Why the hell is this a char ptr?
            out_value = strtof(str.begin(), &ptr_end);
            ICE_ASSERT_CORE(ice::ptr_distance(str.begin(), ptr_end).value <= str.size());
            fc_res.ptr = ptr_end;
#else
            fc_res = std::from_chars(
                ice::string::begin(str),
                ice::string::end(str),
                out_value,
                std::chars_format::general
            );
#endif
        }

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
            .remaining = ice::String{ fc_res.ptr, str.end() }
        };
    }

    template<typename T>
        requires (std::is_integral_v<T> || std::is_floating_point_v<T>)
    auto from_chars(char const* str_beg, char const* str_end, T& out_value) noexcept -> ice::FromCharsResult<char const*>
    {
        ice::ErrorCode res = ice::S_Ok;
        std::from_chars_result fc_res;
        if constexpr (std::is_integral_v<T>)
        {
            fc_res = std::from_chars(str_beg, str_end, out_value);
        }
        else
        {
#if ISP_COMPILER_CLANG <= 20 || ISP_WEBAPP || ISP_ANDROID
            // Because Libc++ did not support from_chars for floats up until clang.20 we need to use the old C style approach...
            // We don't try to handle errors in this version.
            fc_res.ec = std::errc{};
            char* ptr_end = nullptr; // Why the hell is this a char ptr?
            out_value = strtof(str_beg, &ptr_end);
            ICE_ASSERT_CORE(ice::ptr_distance(str_beg, ptr_end) <= ice::ptr_distance(str_beg, str_end));
            fc_res.ptr = ptr_end;
#else
            fc_res = std::from_chars(str_beg, str_end, out_value, std::chars_format::general);
#endif
        }

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
            ice::string::ResizableStringType auto& str,
            fmt::format_string<Args...> format,
            Args&&... args
        ) noexcept
        {
            ice::ncount const pushed_size = fmt::formatted_size(format, ice::forward<Args>(args)...);
            ice::ncount const final_size = str.size() + pushed_size;
            if (final_size + 1 >= str.capacity())
            {
                str.grow(final_size + 1);
            }
            fmt::format_to_n(str.end(), pushed_size, format, ice::forward<Args>(args)...);
            str.resize(final_size);
        }

        template<ice::u32 Capacity, typename... Args>
        constexpr void push_format(
            ice::StaticString<Capacity, char>& str,
            fmt::format_string<Args...> format,
            Args&&... args
        ) noexcept
        {
            ice::ncount const pushed_size = fmt::formatted_size(format, ice::forward<Args>(args)...);
            ice::ncount const final_size = str.size() + pushed_size;
            if (final_size + 1 >= Capacity)
            {
                final_size = Capacity - 1;
            }
            fmt::format_to_n(str.end(), final_size, format, ice::forward<Args>(args)...);
            str.resize(final_size);
        }

        template<typename Fn>
        constexpr auto for_each_split(ice::String contents, ice::String separator, Fn&& fn) noexcept -> ice::u32
        {
            ice::u32 count = 0;
            while(contents.not_empty())
            {
                count += 1;
                ice::nindex const separator_pos = contents.find_first_of(separator);
                ice::String const line = contents.substr(0, separator_pos);
                if (ice::forward<Fn>(fn)(line) == false)
                {
                    break;
                }
                contents = contents.substr(separator_pos + 1);
            }
            return count;
        }

    } // namespace string

} // namespace ice
