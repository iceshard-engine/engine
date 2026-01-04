/// Copyright 2023 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/string_utils.hxx>
#include <ice/os/windows.hxx>
#include <ice/assert.hxx>

namespace ice
{

    auto compare(
        ice::String left,
        ice::String right,
        ice::u64 count,
        ice::CaseSensitive check_case /*= CaseSensitive::No*/
    ) noexcept -> ice::CompareResult
    {
#if ISP_WINDOWS
        ice::u32 const comp_result = check_case == CaseSensitive::Yes
            ? strncmp(left.begin(), right.begin(), count)
            : strnicmp(left.begin(), right.begin(), count);
#elif ISP_UNIX
        ice::u32 const comp_result = check_case == CaseSensitive::Yes
            ? strncmp(ice::string::begin(left), ice::string::begin(right), count)
            : strncasecmp(ice::string::begin(left), ice::string::begin(right), count);
#endif
        return comp_result == 0 ? CompareResult::Equal :
            (comp_result < 0 ? CompareResult::Smaller : CompareResult::Larger);
    }

    auto compare(
        ice::String left,
        ice::String right,
        ice::CaseSensitive check_case /*= CaseSensitive::No*/
    ) noexcept -> ice::CompareResult
    {
        ice::ncount const max_size = ice::min(left.size(), right.size());
#if ISP_WINDOWS
        ice::u32 const comp_result = check_case == CaseSensitive::Yes
            ? strncmp(left.begin(), right.begin(), max_size)
            : strnicmp(left.begin(), right.begin(), max_size);
#elif ISP_UNIX
        ice::u32 const comp_result = check_case == CaseSensitive::Yes
            ? strncmp(ice::string::begin(left), ice::string::begin(right), max_size)
            : strncasecmp(ice::string::begin(left), ice::string::begin(right), max_size);
#endif
        return comp_result == 0 ? CompareResult::Equal :
            (comp_result < 0 ? CompareResult::Smaller : CompareResult::Larger);
    }

    auto utf8_to_wide_size(
        ice::String utf8_str
    ) noexcept -> ice::u32
    {
#if ISP_WINDOWS
        ice::i32 const chars_written = MultiByteToWideChar(
            CP_UTF8,
            0,
            utf8_str.begin(),
            utf8_str.size().u32(),
            nullptr,
            0
        );
        return chars_written;
#else
        ICE_ASSERT(false, "Not implemented!");
        return 0;
#endif
    }

    bool utf8_to_wide_append(
        ice::String utf8_str,
        ice::HeapString<ice::wchar>& out_str
    ) noexcept
    {
#if ISP_WINDOWS
        ice::ncount const required_size = MultiByteToWideChar(
            CP_UTF8,
            0,
            utf8_str.begin(),
            utf8_str.size().u32(),
            NULL,
            0
        );

        if (required_size != 0)
        {
            ice::ncount const current_size = out_str.size();
            ice::ncount const total_size = required_size + out_str.size();
            out_str.resize(total_size);

            ice::ncount const chars_written = MultiByteToWideChar(
                CP_UTF8,
                0,
                utf8_str.begin(),
                utf8_str.size().u32(),
                out_str.begin() + current_size,
                out_str.size().u32() - current_size.u32()
            );

            ICE_ASSERT(
                chars_written == required_size,
                "The predicted string size seems to be off! [{} != {}]",
                chars_written,
                required_size
            );
        }

        return required_size != 0;
#else
        ICE_ASSERT(false, "Not implemented!");
        return false;
#endif
    }

    auto utf8_to_wide(
        ice::Allocator& alloc,
        ice::String path
    ) noexcept -> ice::HeapString<ice::wchar>
    {
        ice::HeapString<ice::wchar> result{ alloc };
        utf8_to_wide_append(path, result);
        return result;
    }


    auto wide_to_utf8_size(
        ice::WString path
    ) noexcept -> ice::u32
    {
#if ISP_WINDOWS
        ice::i32 const required_size = WideCharToMultiByte(
            CP_UTF8,
            0,
            path.begin(),
            path.size().u32(),
            NULL,
            0,
            NULL,
            NULL
        );
        return static_cast<ice::u32>(required_size);
#else
        ICE_ASSERT(false, "Not implemented!");
        return 0;
#endif
    }

    bool wide_to_utf8_append(
        ice::WString path,
        ice::HeapString<>& out_str
    ) noexcept
    {
#if ISP_WINDOWS
        ice::ncount const required_size = wide_to_utf8_size(path);
        if (required_size != 0)
        {
            ice::ncount const current_size = out_str.size();
            ice::ncount const total_size = required_size + out_str.size();
            out_str.resize(total_size);

            ice::i32 const chars_written = WideCharToMultiByte(
                CP_UTF8,
                0,
                path.begin(),
                path.size().u32(),
                out_str.begin() + current_size,
                out_str.size().u32() - current_size.u32(),
                NULL,
                NULL
            );

            ICE_ASSERT(
                chars_written == required_size,
                "The predicted string size seems to be off! [{} != {}]",
                chars_written,
                required_size
            );
        }

        return required_size != 0;
#else
        ICE_ASSERT(false, "Not implemented!");
        return false;
#endif
    }

    auto wide_to_utf8(
        ice::Allocator& alloc,
        ice::WString path
    ) noexcept -> ice::HeapString<>
    {
        ice::HeapString<> result{ alloc };
        wide_to_utf8_append(path, result);
        return result;
    }


} // namespace ice
