/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "linux_storage.hxx"

#include <ice/mem_allocator_stack.hxx>
#include <ice/string_utils.hxx>
#include <ice/path_utils.hxx>
#include <ice/os/windows.hxx>
#include <ice/app_info.hxx>
#include <ice/log.hxx>

namespace ice::platform::linux
{

    namespace detail
    {

        static constexpr ice::String FolderID_Desktop = "XDG_DESKTOP_DIR";
        static constexpr ice::String FolderID_Download = "XDG_DOWNLOAD_DIR";
        static constexpr ice::String FolderID_Templates = "XDG_TEMPLATES_DIR";
        static constexpr ice::String FolderID_Publicshare = "XDG_PUBLICSHARE_DIR";
        static constexpr ice::String FolderID_Documents = "XDG_DOCUMENTS_DIR";
        static constexpr ice::String FolderID_Music = "XDG_MUSIC_DIR";
        static constexpr ice::String FolderID_Pictures = "XDG_PICTURES_DIR";
        static constexpr ice::String FolderID_SavedGames = "XDG_SAVEDGAMES_DIR";
        static constexpr ice::String FolderID_Videos = "XDG_VIDEOS_DIR";

        void create_known_path(ice::HeapString<>& out_path, ice::String home, ice::String known_path, ice::String appname) noexcept
        {
            ice::string::push_format(out_path, "{}/{}/{}", home, known_path, appname);
            ice::path::normalize(out_path);
        }

        void find_known_path(ice::HeapString<>& out_path, ice::String contents, ice::String home, ice::String known_path_id, ice::String fallback, ice::String appname) noexcept
        {
            ice::string::for_each_split(
                contents,
                "\n",
                [&](ice::String line) noexcept
                {
                    if (ice::string::empty(line) || line[0] == '#')
                    {
                        return true;
                    }

                    ice::ucount const idassignment = ice::string::find_first_of(line, '=');
                    if (idassignment == ice::none_index)
                    {
                        ICE_LOG(LogSeverity::Warning, LogTag::Core, "Improperly formatted line: {}", line);
                        return true;
                    }

                    ice::String const id = ice::string::substr(line, 0, idassignment);
                    ice::ucount const value_start = ice::string::find_first_of(line, '"', idassignment);
                    ice::ucount const value_end = ice::string::find_last_of(line, '"');
                    if (value_end == value_start)
                    {
                        ICE_LOG(LogSeverity::Warning, LogTag::Core, "Improperly formatted line: {}", line);
                        return true;
                    }

                    ice::String const value = ice::string::substr(line, value_start + 1, (value_end - value_start) - 1);
                    ICE_LOG(LogSeverity::Debug, LogTag::Core, "- {} = {}", id, value);

                    if (id == known_path_id)
                    {
                        create_known_path(out_path, home, ice::string::substr(value, 5), appname);
                        ICE_LOG(LogSeverity::Info, LogTag::Core, "Known path ID {} found: {}", known_path_id, out_path);
                        return false;
                    }
                    return true;
                }
            );

            if (ice::string::empty(out_path))
            {
                create_known_path(out_path, home, fallback, appname);
                ICE_LOG(LogSeverity::Info, LogTag::Core, "Known path ID {} not found, using fallback: {}", known_path_id, out_path);
            }
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
        : _allocator{ alloc }
        , _config_file{ alloc }
        , _home_location{ alloc }
        , _save_location{ alloc }
        , _cache_location{ alloc }
        , _temp_location{ alloc }
        , _pictures_location{ alloc }
        , _other_location{ alloc }
    {
        // Get HOME securely
        char const* home_env = secure_getenv("HOME");
        ICE_ASSERT_CORE(home_env != nullptr);
        _home_location = home_env;


        char const* app_name = "iceshard";
        for (ice::Shard param : params)
        {
            if (param == Shard_StorageAppName)
            {
                ice::shard_inspect(param, app_name);
                // TODO: Warning if shard value was not valid.
            }
        }

        // Hardcoded paths
        _config_file = ice::native_file::path_from_strings(alloc, _home_location, ".config/user-dirs.dirs");
        _temp_location = ice::native_file::path_from_strings(alloc, "/tmp", app_name);
        _cache_location = ice::native_file::path_from_strings(alloc, _home_location, ".iceshard", app_name);

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
        ice::string::clear(_pictures_location);
        ice::string::clear(_other_location);

        ICE_LOG(LogSeverity::Info, LogTag::Core, "Checking for XDG {} file...", _config_file);
        if (ice::native_file::exists_file(_config_file))
        {
            ICE_LOG(LogSeverity::Info, LogTag::Core, "Found, parsing for known directories...");
            ice::native_file::File const file = ice::native_file::open_file(_config_file);
            ice::Memory const contents_mem = _allocator.allocate(ice::native_file::sizeof_file(file));
            ice::String contents = ice::String{ reinterpret_cast<char const*>(contents_mem.location), ice::ucount(contents_mem.size.value) };

            if (ice::native_file::read_file(file, contents_mem.size, contents_mem) > 0_B)
            {
                detail::find_known_path(_save_location, contents, _home_location, detail::FolderID_SavedGames, "SavedGames", appname);
                detail::find_known_path(_pictures_location, contents, _home_location, detail::FolderID_Pictures, "Pictures", appname);
                detail::find_known_path(_other_location, contents, _home_location, detail::FolderID_Documents, "Documents", appname);
            }

            _allocator.deallocate(contents_mem);
        }
        else
        {
            ICE_LOG(LogSeverity::Info, LogTag::Core, "File does not exist, falling back to expected defaults", _config_file);
            detail::create_known_path(_save_location, _home_location, "SavedGames", appname);
            detail::create_known_path(_pictures_location, _home_location, "Pictures", appname);
            detail::create_known_path(_other_location, _home_location, "Documents", appname);
        }

        ice::native_file::create_directory(_temp_location);
        ice::native_file::create_directory(_cache_location);
        ice::native_file::create_directory(_save_location);
        ice::native_file::create_directory(_pictures_location);
        ice::native_file::create_directory(_other_location);
    }

} // namespace ice::platform::linux
