/// Copyright 2023 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/assert.hxx>
#include <ice/app_info.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <ice/string_utils.hxx>
#include <ice/native_file.hxx>

namespace ice::app
{

    auto version() noexcept -> ice::app::Version
    {
        return { .major = 0, .minor = 0, .patch = 0, .build = 0, .commit = { 0, 0, 0, 0, 0 } };
    }

    auto name() noexcept -> ice::String
    {
        return { "iceshard-application" };
    }

#if ISP_WINDOWS
    auto location() noexcept -> ice::Path
    {
        static ice::StaticString<256> app_location = []() noexcept
        {
            ice::StaticString<256, ice::wchar> location_wide{ L"" };
            DWORD const path_size = GetModuleFileNameW(NULL, location_wide.begin(), location_wide.capacity().u32());
            location_wide.resize(path_size);

            ice::StackAllocator_1024 stack_alloc;
            ice::HeapString<> location_utf8{ stack_alloc };
            ice::wide_to_utf8_append(location_wide, location_utf8);

            return ice::StaticString<256>{ location_utf8 };
        }();

        return ice::Path{ app_location };
    }

    auto directory() noexcept -> ice::Path
    {
        static ice::Path app_directory = location().directory();
        return app_directory;
    }

    auto workingdir() noexcept -> ice::Path
    {
        static ice::StaticString<256> working_dir = []() noexcept
        {
            ice::StaticString<256, ice::wchar> location_wide{ L"" };
            DWORD const path_size = GetCurrentDirectoryW(location_wide.capacity().u32(), location_wide.begin());
            location_wide.resize(path_size);

            ice::StackAllocator_1024 stack_alloc;
            ice::HeapString<> location_utf8{ stack_alloc };
            ice::wide_to_utf8_append(location_wide, location_utf8);

            return ice::StaticString<256>{ location_utf8 };
        }();

        return ice::Path{ working_dir };
    }
#elif ISP_LINUX
    auto location() noexcept -> ice::Path
    {
        static ice::StaticString<PATH_MAX> app_location = []() noexcept
        {
            ice::StaticString<PATH_MAX> result{ "" };
            int nchar = readlink("/proc/self/exe", result.begin(), result.capacity());
            result.resize(nchar);
            return result;
        }();

        return ice::Path{ app_location };
    }

    auto directory() noexcept -> ice::Path
    {
        static ice::Path app_directory = location().directory();
        return app_directory;
    }

    auto workingdir() noexcept -> ice::Path
    {
        static ice::StaticString<PATH_MAX> working_dir = []() noexcept
        {
            ice::StaticString<PATH_MAX> result{};
            char const* success = getcwd(result.begin(), result.capacity());
            ICE_ASSERT(success != nullptr, "Current working directory is too long, can't contain the value!");
            result.resize(std::strlen(success));

            return ice::StaticString<PATH_MAX>{ result };
        }();

        return ice::Path{ working_dir };
    }
#else
    auto location() noexcept -> ice::Path
    {
        ICE_ASSERT_CORE(false);
        return {};
    }

    auto directory() noexcept -> ice::Path
    {
        ICE_ASSERT_CORE(false);
        return {};
    }

    auto workingdir() noexcept -> ice::Path
    {
        ICE_ASSERT_CORE(false);
        return {};
    }
#endif

} // namespace ice::app
