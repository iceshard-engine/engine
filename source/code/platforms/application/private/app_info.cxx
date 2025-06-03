/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/assert.hxx>
#include <ice/app_info.hxx>
#include <ice/string/static_string.hxx>

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
#else
    auto location() noexcept -> ice::String
    {
        ICE_ASSERT_CORE(false);
        return {};
    }

    auto directory() noexcept -> ice::String
    {
        ICE_ASSERT_CORE(false);
        return {};
    }

    auto workingdir() noexcept -> ice::String
    {
        static ice::StaticString<256> working_dir = []() noexcept
        {
            ice::StaticString<256> result{};
            char const* success = getcwd(ice::string::begin(result), ice::string::capacity(result));
            ICE_ASSERT(success != nullptr, "Current working directory is too long, can't contain the value!");
            ice::string::resize(result, std::strlen(success));

            return ice::StaticString<256>{ result };
        }();

        return working_dir;
    }
#endif

} // namespace ice::app
