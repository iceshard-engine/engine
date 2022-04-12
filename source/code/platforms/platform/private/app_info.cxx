#include <ice/app_info.hxx>
#include <ice/heap_string.hxx>
#include <ice/stack_string.hxx>
#include <ice/os/windows.hxx>
#include <ice/os/unix.hxx>
#include <ice/assert.hxx>

namespace ice
{

#if ISP_WINDOWS

    void wide_to_utf8(ice::WString path, ice::HeapString<char8_t>& out_str) noexcept
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

        if (required_size != 0)
        {
            ice::u32 const current_size = ice::string::size(out_str);
            ice::u32 const total_size = static_cast<ice::u32>(required_size) + ice::string::size(out_str);
            ice::string::resize(out_str, total_size);

            ice::i32 const chars_written = WideCharToMultiByte(
                CP_UTF8,
                0,
                ice::string::data(path),
                static_cast<ice::i32>(ice::string::size(path)),
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
    }

    void app_location(ice::HeapString<char8_t>& out) noexcept
    {
        ice::StackString<256, wchar_t> temp{ L"" };
        DWORD const path_size = GetModuleFileNameW(NULL, ice::string::begin(temp), ice::string::capacity(temp));
        ice::string::resize(temp, path_size);

        wide_to_utf8(temp, out);
    }

    void working_directory(ice::HeapString<char8_t>& out) noexcept
    {
        ice::StackString<256, wchar_t> temp{ L"" };
        DWORD const path_size = GetCurrentDirectoryW(ice::string::capacity(temp), ice::string::begin(temp));
        ice::string::resize(temp, path_size);

        wide_to_utf8(temp, out);
    }

#else

    void location(ice::HeapString<>& out) noexcept
    {
        ice::string::clear(out);
    }

    void working_directory(ice::HeapString<>& out) noexcept
    {
        ice::string::clear(out);
    }

#endif

} // namespace core
