/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "resource_utils_win32.hxx"
#include <ice/mem_allocator_stack.hxx>
#include <ice/string/heap_string.hxx>
#include <ice/assert.hxx>

#if ISP_WINDOWS

namespace ice::win32
{

    auto native_open_file(
        ice::WString path, int flags
    ) noexcept -> ice::win32::FileHandle
    {
        ice::win32::FileHandle handle = CreateFile(
            ice::string::begin(path),
            FILE_READ_DATA,
            FILE_SHARE_READ, // FILE_SHARE_*
            NULL, // SECURITY ATTRIBS
            OPEN_EXISTING,
            flags,
            NULL
        );
        return handle;
    }

    bool native_load_file(
        ice::win32::FileHandle const& handle,
        ice::Allocator& alloc,
        ice::Memory& out_data
    ) noexcept
    {
        BOOL result = FALSE;
        LARGE_INTEGER li;
        if (GetFileSizeEx(handle.native(), &li) != 0)
        {
            out_data = alloc.allocate({ { ice::usize::base_type(li.QuadPart) }, ice::ualign::b_default });

            DWORD characters_read = 0;
            do
            {
                DWORD const characters_to_read = (DWORD)out_data.size.value;
                ICE_ASSERT(
                    characters_to_read == out_data.size.value,
                    "File is larger than this function can handle! For now... [file size: {}]",
                    out_data.size
                );

                result = ReadFile(
                    handle.native(),
                    out_data.location,
                    (DWORD)out_data.size.value,
                    &characters_read,
                    nullptr
                );

                ICE_ASSERT(
                    characters_read == characters_to_read,
                    "Read different amount of characters to what was expected. [expected: {}, read: {}]",
                    characters_to_read,
                    characters_read
                );

            } while (characters_read == 0 && result != FALSE);
        }
        return result != FALSE;
    }

} // namespace ice

#endif // #if ISP_WINDOWS
