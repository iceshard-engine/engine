/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/string_types.hxx>
//#include <ice/buffer.hxx>
#include <ice/os/windows.hxx>
#include <ice/path_utils.hxx>

#if ISP_WINDOWS

namespace ice::win32
{

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
