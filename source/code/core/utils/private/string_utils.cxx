/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/string_utils.hxx>
#include <ice/os/windows.hxx>
#include <ice/assert.hxx>

namespace ice
{

    auto utf8_to_wide_size(
        ice::String utf8_str
    ) noexcept -> ice::u32
    {
#if ISP_WINDOWS
        ice::i32 const chars_written = MultiByteToWideChar(
            CP_UTF8,
            0,
            ice::string::begin(utf8_str),
            ice::string::size(utf8_str),
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
        ice::i32 const required_size = MultiByteToWideChar(
            CP_UTF8,
            0,
            ice::string::begin(utf8_str),
            ice::string::size(utf8_str),
            NULL,
            0
        );

        if (required_size != 0)
        {
            ice::u32 const current_size = ice::string::size(out_str);
            ice::u32 const total_size = static_cast<ice::u32>(required_size) + ice::string::size(out_str);
            ice::string::resize(out_str, total_size);

            ice::i32 const chars_written = MultiByteToWideChar(
                CP_UTF8,
                0,
                ice::string::begin(utf8_str),
                ice::string::size(utf8_str),
                ice::string::begin(out_str) + current_size,
                ice::string::size(out_str) - current_size
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
            ice::string::begin(path),
            ice::string::size(path),
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
        ice::i32 const required_size = wide_to_utf8_size(path);
        if (required_size != 0)
        {
            ice::u32 const current_size = ice::string::size(out_str);
            ice::u32 const total_size = static_cast<ice::u32>(required_size) + ice::string::size(out_str);
            ice::string::resize(out_str, total_size);

            ice::i32 const chars_written = WideCharToMultiByte(
                CP_UTF8,
                0,
                ice::string::begin(path),
                ice::string::size(path),
                ice::string::begin(out_str) + current_size,
                ice::string::size(out_str) - current_size,
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
