/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/string_types.hxx>
//#include <ice/buffer.hxx>
#include <ice/os/windows.hxx>

#include "../path_utils.hxx"

#if ISP_WINDOWS

namespace ice::win32
{

    bool utf8_to_wide_append(
        ice::String path,
        ice::HeapString<ice::wchar>& out_str
    ) noexcept;

    auto utf8_to_wide(
        ice::Allocator& alloc,
        ice::String path
    ) noexcept -> ice::HeapString<ice::wchar>;

    auto wide_to_utf8_size(
        ice::WString path
    ) noexcept -> ice::u32;

    bool wide_to_utf8(
        ice::WString path,
        ice::HeapString<>& out_str
    ) noexcept;

    auto native_open_file(
        ice::WString path,
        int flags
    ) noexcept -> ice::win32::FileHandle;

    bool native_load_file(
        ice::win32::FileHandle const& handle,
        ice::Allocator& alloc,
        ice::Memory& out_data
    ) noexcept;

} // namespace ice

#endif // #if ISP_WINDOWS
