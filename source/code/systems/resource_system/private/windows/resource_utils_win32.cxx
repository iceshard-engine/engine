#include "resource_utils_win32.hxx"
#include <ice/memory/stack_allocator.hxx>
#include <ice/heap_string.hxx>
#include <ice/string.hxx>
#include <ice/assert.hxx>

#if ISP_WINDOWS

namespace ice
{

    bool utf8_to_wide_append(
        ice::Utf8String path,
        ice::HeapString<wchar_t>& out_str
    ) noexcept
    {
        ice::i32 const required_size = MultiByteToWideChar(
            CP_UTF8,
            0,
            reinterpret_cast<char const*>(ice::string::data(path)),
            static_cast<ice::i32>(ice::string::size(path)),
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
                (const char*)ice::string::data(path),
                ice::as_i32<NC_SIGN>(ice::string::size(path)),
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
        ice::Utf8String path
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
            ice::string::data(path),
            static_cast<ice::i32>(ice::string::size(path)),
            NULL,
            0,
            NULL,
            NULL
        );
        return static_cast<ice::u32>(required_size);
    }

    bool wide_to_utf8(
        ice::WString path,
        ice::HeapString<char8_t>& out_str
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
                ice::string::data(path),
                ice::as_i32<NC_SIGN>(ice::string::size(path)),
                (char*)ice::string::begin(out_str) + current_size,
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


    auto win32_open_file(
        ice::WString path, int flags
    ) noexcept -> ice::win32::SHHandle
    {
        ice::win32::SHHandle handle = CreateFile(
            ice::string::data(path),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE, // FILE_SHARE_*
            NULL, // SECURITY ATTRIBS
            OPEN_EXISTING,
            flags,
            NULL
        );
        return handle;
    }

    bool win32_load_file(
        ice::win32::SHHandle const& handle,
        ice::Buffer& out_buffer
    ) noexcept
    {
        ice::memory::StackAllocator_4096 kib4_alloc;
        ice::Memory memory_chunk = {
            .location = kib4_alloc.allocate(kib4_alloc.Constant_BufferSize),
            .size = kib4_alloc.Constant_BufferSize,
            .alignment = 1 // Characters are not aligned to any particular number
        };

        BOOL result = FALSE;
        DWORD characters_read = 0;
        do
        {
            result = ReadFile(
                handle.native(),
                memory_chunk.location,
                memory_chunk.size,
                &characters_read,
                nullptr
            );

            if (characters_read > 0)
            {
                ice::buffer::append(
                    out_buffer,
                    memory_chunk.location,
                    characters_read,
                    memory_chunk.alignment
                );
            }

        } while (characters_read == 0 && result != FALSE);

        kib4_alloc.deallocate(memory_chunk.location);
        return result != FALSE;
    }

} // namespace ice

#endif // #if ISP_WINDOWS
