/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "linux_storage.hxx"

#include <ice/mem_allocator_stack.hxx>
#include <ice/string_utils.hxx>
#include <ice/path_utils.hxx>
#include <ice/os/windows.hxx>
#include <ice/app_info.hxx>

namespace ice::platform::linux
{

    static constexpr ice::ucount Constant_AppNameReservedLength = 24;

    namespace detail
    {

        void get_known_path(ice::HeapString<>& out_path, char const* known_path_id, ice::String appname = {})
        {
        }

        bool create_directory(ice::String path) noexcept
        {
            return false;
        }

    } // namespace detail

    LinuxStorage::LinuxStorage(
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

    bool LinuxStorage::set_appname(ice::String name) noexcept
    {
        // reload_paths(name);

        // TODO: Handle errors?
        // detail::create_directory(_save_location);
        // detail::create_directory(_cache_location);
        // detail::create_directory(_pictures_location);
        // detail::create_directory(_other_location);
        return false;
    }

    auto LinuxStorage::data_locations() const noexcept -> ice::Span<ice::String const>
    {
        static ice::String paths[]{
            ice::app::directory()
        };
        return paths;
    }

    auto LinuxStorage::dylibs_location() const noexcept -> ice::String
    {
        return ice::path::directory(ice::app::location());
    }

    void LinuxStorage::reload_paths(ice::String appname) noexcept
    {
        ice::string::clear(_save_location);
        ice::string::clear(_cache_location);
        ice::string::clear(_temp_location);
        ice::string::clear(_pictures_location);
        ice::string::clear(_other_location);

        // detail::get_known_path(_save_location, FOLDERID_SavedGames, appname);
        // detail::get_known_path(_cache_location, FOLDERID_LocalAppData, appname);
        // detail::get_known_path(_pictures_location, FOLDERID_Pictures, appname);
        // detail::get_known_path(_other_location, FOLDERID_Documents, appname);

        detail::create_directory(_save_location);
        detail::create_directory(_cache_location);
        detail::create_directory(_pictures_location);
        detail::create_directory(_other_location);

        // WCHAR tempbuff[256];
        // // GetTempPathW already returns a path ending with a slash character.
        // DWORD const len = GetTempPathW(256, tempbuff);
        // ice::wide_to_utf8_append({ tempbuff, ice::ucount(len) }, _temp_location);
        // ice::path::normalize(_temp_location);
    }

} // namespace ice::platform::linux
