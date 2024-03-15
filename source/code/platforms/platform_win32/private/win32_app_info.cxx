/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/app_info.hxx>

#if ISP_WINDOWS
#include <ice/os/windows.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <ice/string/static_string.hxx>
#include <ice/string_utils.hxx>
#include <ice/path_utils.hxx>

namespace ice::app
{

    auto location() noexcept -> ice::String
    {
        static ice::StaticString<256> app_location = []() noexcept
        {
            ice::StaticString<256, ice::wchar> location_wide{ L"" };
            DWORD const path_size = GetModuleFileNameW(NULL, ice::string::begin(location_wide), ice::string::capacity(location_wide));
            ice::string::resize(location_wide, path_size);

            ice::StackAllocator_1024 stack_alloc;
            ice::HeapString<> location_utf8{ stack_alloc };
            ice::wide_to_utf8_append(location_wide, location_utf8);

            return ice::StaticString<256>{ location_utf8 };
        }();

        return app_location;
    }

    auto directory() noexcept -> ice::String
    {
        static ice::String app_directory = ice::path::directory(location());
        return app_directory;
    }

    auto workingdir() noexcept -> ice::String
    {
        static ice::StaticString<256> working_dir = []() noexcept
        {
            ice::StaticString<256, ice::wchar> location_wide{ L"" };
            DWORD const path_size = GetCurrentDirectoryW(ice::string::capacity(location_wide), ice::string::begin(location_wide));
            ice::string::resize(location_wide, path_size);

            ice::StackAllocator_1024 stack_alloc;
            ice::HeapString<> location_utf8{ stack_alloc };
            ice::wide_to_utf8_append(location_wide, location_utf8);

            return ice::StaticString<256>{ location_utf8 };
        }();

        return working_dir;
    }

} // namespace ice::app

#endif // ISP_WINDOWS
