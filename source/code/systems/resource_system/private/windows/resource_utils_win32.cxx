#include "resource_utils_win32.hxx"
#include <ice/mem_allocator_stack.hxx>
#include <ice/string/heap_string.hxx>
#include <ice/assert.hxx>

#if ISP_WINDOWS

namespace ice::win32
{

    bool utf8_to_wide_append(
        ice::String path,
        ice::HeapString<wchar_t>& out_str
    ) noexcept
    {
        ice::i32 const required_size = MultiByteToWideChar(
            CP_UTF8,
            0,
            ice::string::begin(path),
            ice::string::size(path),
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
                ice::string::begin(path),
                ice::string::size(path),
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
    }

    auto utf8_to_wide(
        ice::Allocator& alloc,
        ice::String path
    ) noexcept -> ice::HeapString<wchar_t>
    {
        ice::HeapString<wchar_t> result{ alloc };
        utf8_to_wide_append(path, result);
        return result;
    }


    auto wide_to_utf8_size(
        ice::WString path
    ) noexcept -> ice::u32
    {
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
    }

    bool wide_to_utf8(
        ice::WString path,
        ice::HeapString<>& out_str
    ) noexcept
    {
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
    }


    auto native_open_file(
        ice::WString path, int flags
    ) noexcept -> ice::win32::FileHandle
    {
        ice::win32::FileHandle handle = CreateFile(
            ice::string::begin(path),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE, // FILE_SHARE_*
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
