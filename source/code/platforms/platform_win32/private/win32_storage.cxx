/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "win32_storage.hxx"

#include <ice/mem_allocator_stack.hxx>
#include <ice/string_utils.hxx>
#include <ice/path_utils.hxx>
#include <ice/os/windows.hxx>
#include <ice/app_info.hxx>
#include <shlobj.h>
#include <shellapi.h>
#include <shlwapi.h>

#if ISP_WINDOWS

namespace ice::platform::win32
{

    static constexpr ice::ucount Constant_AppNameReservedLength = 24;

    namespace detail
    {

        void get_known_path(ice::HeapString<>& out_path, const GUID& known_path_guid, ice::String appname = {})
        {
            PWSTR path;
            if (SHGetKnownFolderPath(known_path_guid, KF_FLAG_CREATE, NULL, &path) == S_OK)
            {
                ice::ucount const path_len = ice::ucount(lstrlenW(path));
                // Reserve additional space for the app name, so we don't need to resize buffers if that happens.
                ice::string::reserve(out_path, path_len + Constant_AppNameReservedLength);
                ice::wide_to_utf8_append({ path, ice::ucount(lstrlenW(path)) }, out_path);
                ice::string::push_back(out_path, '\\');
                if (ice::string::any(appname))
                {
                    ice::string::push_back(out_path, appname);
                    ice::string::push_back(out_path, '\\');
                }
                ice::path::normalize(out_path);
                CoTaskMemFree(path);
            }
        }

        bool create_directory(ice::String path) noexcept
        {
            ice::StackAllocator<512_B> temp_alloc;
            ice::HeapString<ice::wchar> temp_paths{ temp_alloc };
            ice::string::reserve(temp_paths, 256);
            ice::utf8_to_wide_append(path, temp_paths);
            return CreateDirectoryW(ice::string::begin(temp_paths), NULL);
        }

    } // namespace detail

    Win32Storage::Win32Storage(
        ice::Allocator& alloc,
        ice::Span<ice::Shard const> params
    ) noexcept
        : _save_location{ alloc }
        , _cache_location{ alloc }
        , _temp_location{ alloc }
        , _pictures_location{ alloc }
        , _other_location{ alloc }
    {
        char const* app_name = "";
        for (ice::Shard param : params)
        {
            if (param == Shard_StorageAppName)
            {
                ice::shard_inspect(param, app_name);
                // TODO: Warning if shard value was not valid.
            }
        }

        // TODO: Use a special function that can be overriden by the host app?
        reload_paths(app_name);
    }

    bool Win32Storage::set_appname(ice::String name) noexcept
    {
        // reload_paths(name);

        // TODO: Handle errors?
        // detail::create_directory(_save_location);
        // detail::create_directory(_cache_location);
        // detail::create_directory(_pictures_location);
        // detail::create_directory(_other_location);
        return false;
    }

    auto win32::Win32Storage::data_locations() const noexcept -> ice::Span<ice::String const>
    {
        static ice::String paths[]{
            ice::app::directory()
        };
        return paths;
    }

    auto win32::Win32Storage::dylibs_location() const noexcept -> ice::String
    {
        return ice::path::directory(ice::app::location());
    }

    void win32::Win32Storage::reload_paths(ice::String appname) noexcept
    {
        ice::string::clear(_save_location);
        ice::string::clear(_cache_location);
        ice::string::clear(_temp_location);
        ice::string::clear(_pictures_location);
        ice::string::clear(_other_location);

        detail::get_known_path(_save_location, FOLDERID_SavedGames, appname);
        detail::get_known_path(_cache_location, FOLDERID_LocalAppData, appname);
        detail::get_known_path(_pictures_location, FOLDERID_Pictures, appname);
        detail::get_known_path(_other_location, FOLDERID_Documents, appname);

        detail::create_directory(_save_location);
        detail::create_directory(_cache_location);
        detail::create_directory(_pictures_location);
        detail::create_directory(_other_location);

        WCHAR tempbuff[256];
        // GetTempPathW already returns a path ending with a slash character.
        DWORD const len = GetTempPathW(256, tempbuff);
        ice::wide_to_utf8_append({ tempbuff, ice::ucount(len) }, _temp_location);
        ice::path::normalize(_temp_location);
    }

} // namespace ice

#endif
