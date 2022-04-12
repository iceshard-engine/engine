#pragma once
#include <ice/allocator.hxx>
#include <ice/string_types.hxx>
#include <ice/buffer.hxx>
#include <ice/os/windows.hxx>

#include "../path_utils.hxx"

#if ISP_WINDOWS

namespace ice
{

    bool utf8_to_wide_append(
        ice::Utf8String path,
        ice::HeapString<wchar_t>& out_str
    ) noexcept;

    auto utf8_to_wide(
        ice::Allocator& alloc,
        ice::Utf8String path
    ) noexcept -> ice::HeapString<wchar_t>;

    auto wide_to_utf8_size(
        ice::WString path
    ) noexcept -> ice::u32;

    bool wide_to_utf8(
        ice::WString path,
        ice::HeapString<char8_t>& out_str
    ) noexcept;

    auto win32_open_file(
        ice::WString path,
        int flags
    ) noexcept -> ice::win32::SHHandle;

    bool win32_load_file(
        ice::win32::SHHandle const& handle,
        ice::Buffer& out_buffer
    ) noexcept;

} // namespace ice

#endif // #if ISP_WINDOWS
