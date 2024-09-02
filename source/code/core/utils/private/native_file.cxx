/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT
#include <ice/native_file.hxx>
#include <ice/string_utils.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <ice/profiler.hxx>
#include <ice/assert.hxx>

#if ISP_UNIX
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#endif

#if ISP_WEBAPP
#include <emscripten.h>
#endif

namespace ice::native_file
{

#if ISP_WINDOWS

    inline auto translate_disposition(ice::native_file::FileOpenFlags flags) noexcept -> DWORD
    {
        // For details see: https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew
        DWORD result = OPEN_EXISTING;
        if (ice::has_any(flags, FileOpenFlags::Write))
        {
            result = CREATE_ALWAYS;
        }
        return result;
    }

    inline auto translate_mode(ice::native_file::FileOpenFlags flags) noexcept -> DWORD
    {
        // For details see: https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew
        DWORD result = 0;
        if (ice::has_none(flags, FileOpenFlags::Exclusive))
        {
            if (ice::has_any(flags, FileOpenFlags::Write))
            {
                result = FILE_SHARE_WRITE;
            }
            else
            {
                result = FILE_SHARE_READ;
            }
        }
        return result;
    }

    inline auto translate_access(ice::native_file::FileOpenFlags flags) noexcept -> DWORD
    {
        // For details see: https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew
        DWORD result = FILE_READ_ATTRIBUTES;
        if (ice::has_any(flags, FileOpenFlags::Write))
        {
            result |= FILE_WRITE_DATA;
        }
        else
        {
            result |= FILE_READ_DATA;
        }
        return result;
    }

    inline auto translate_attribs(ice::native_file::FileOpenFlags flags) noexcept -> DWORD
    {
        // For details see: https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew
        DWORD result = FILE_ATTRIBUTE_NORMAL;
        if (ice::has_any(flags, FileOpenFlags::Asynchronous))
        {
            result = FILE_FLAG_OVERLAPPED;
        }
        return result;
    }

    bool exists_file(ice::native_file::FilePath path) noexcept
    {
        IPT_ZONE_SCOPED;
        DWORD const result = GetFileAttributesW(ice::string::begin(path));
        return result != INVALID_FILE_ATTRIBUTES
            && (result == FILE_ATTRIBUTE_NORMAL
                || (result & (FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_READONLY)) != 0);
    }

    auto open_file(
        ice::native_file::FilePath path,
        ice::native_file::FileOpenFlags flags /*= FileOpenFlags::ReadOnly*/
    ) noexcept -> ice::native_file::File
    {
        IPT_ZONE_SCOPED;
        ice::win32::FileHandle handle{
            CreateFileW(
                ice::string::begin(path),
                translate_access(flags),
                translate_mode(flags), // FILE_SHARE_*
                NULL, // SECURITY ATTRIBS
                translate_disposition(flags),
                translate_attribs(flags),
                NULL
            )
        };
        return handle;
    }

    auto sizeof_file(ice::native_file::File const& native_file) noexcept -> ice::usize
    {
        LARGE_INTEGER result;
        if (GetFileSizeEx(native_file.native(), &result) == 0)
        {
            result.QuadPart = 0;
        }
        return { static_cast<ice::usize::base_type>(result.QuadPart) };
    }

    auto sizeof_file(ice::native_file::FilePath path) noexcept -> ice::usize
    {
        ice::win32::FileHandle handle{
            CreateFileW(
                path._data,
                FILE_READ_ATTRIBUTES,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
            )
        };
        return sizeof_file(handle);
    }

    auto read_file(
        ice::native_file::File const& native_file,
        ice::usize requested_read_size,
        ice::Memory memory
    ) noexcept -> ice::usize
    {
        return read_file(native_file, 0_B, requested_read_size, memory);
    }

    auto read_file(
        ice::native_file::File const& native_file,
        ice::usize requested_read_offset,
        ice::usize requested_read_size,
        ice::Memory memory
    ) noexcept -> ice::usize
    {
        IPT_ZONE_SCOPED;
        ICE_ASSERT_CORE(memory.size >= requested_read_size);

        if (requested_read_size > 0_B)
        {
            BOOL result;
            DWORD characters_read = 0;
            do
            {
                DWORD const characters_to_read = (DWORD)requested_read_size.value;
                ICE_ASSERT(
                    characters_to_read == requested_read_size.value,
                    "File is larger than this function can handle! For now... [native_file size: {}]",
                    requested_read_size
                );

                LARGE_INTEGER const offset{ .QuadPart = static_cast<ice::isize::base_type>(requested_read_offset.value) };
                OVERLAPPED overlapped{};
                overlapped.Offset = offset.LowPart;
                overlapped.OffsetHigh = offset.HighPart;

                result = ReadFile(
                    native_file.native(),
                    memory.location,
                    characters_to_read,
                    &characters_read,
                    &overlapped
                );

                ICE_ASSERT(
                    characters_read == characters_to_read,
                    "Read different amount of characters to what was expected. [expected: {}, read: {}]",
                    characters_to_read,
                    characters_read
                );

            } while (characters_read == 0 && result != FALSE);
        }
        return requested_read_size;
    }

    auto write_file(
        ice::native_file::File const& native_file,
        ice::usize write_offset,
        ice::Data data
    ) noexcept -> ice::usize
    {
        ice::usize total_written = 0_B;

        OVERLAPPED overlapped{};
        {
            // Set the initial offset at which we want to write.
            // NOTE: If offset is set to 0xffff'ffff'ffff'ffff == Append to end of file. From MSDN docs.
            LARGE_INTEGER const offset{ .QuadPart = (LONGLONG) write_offset.value };
            overlapped.Offset = offset.LowPart;
            overlapped.OffsetHigh = offset.HighPart;
        }

        while(data.size > 0_B)
        {
            // Calculate how much data we should write in a single call (up-to DWORD max value)
            ice::usize::base_type const data_to_write = ice::min<ice::usize::base_type>(
                data.size.value, std::numeric_limits<DWORD>::max()
            );

            DWORD bytes_written = 0;
            BOOL const result = WriteFile(native_file.native(), data.location, (DWORD) data_to_write, &bytes_written, &overlapped);
            ICE_ASSERT(result == TRUE, "Failed to write {} bytes to file.", data_to_write);
            if (result == FALSE)
            {
                break;
            }

            total_written += { bytes_written };

            // Update the offset where we should write. Unless we are always appending
            if (write_offset != ice::usize{ std::numeric_limits<ice::usize::base_type>::max() })
            {
                LARGE_INTEGER const offset{ .QuadPart = (LONGLONG) data_to_write };
                overlapped.Offset = offset.LowPart;
                overlapped.OffsetHigh = offset.HighPart;
            }

            // Move the memory block to the next location to be written
            data = {
                .location = ice::ptr_add(data.location, { data_to_write }),
                .size = ice::usize::subtract(data.size, { data_to_write }),
                .alignment = data.alignment
            };
        }

        return total_written;
    }

    auto append_file(
        ice::native_file::File const& native_file,
        ice::Data data
    ) noexcept -> ice::usize
    {
        // Set offset to usize max == append at file end.
        return write_file(native_file, ice::usize{ std::numeric_limits<ice::usize::base_type>::max() }, data);
    }

    bool traverse_directories_internal(
        ice::native_file::FilePath basepath,
        ice::native_file::HeapFilePath &dirpath,
        ice::native_file::TraversePathCallback callback,
        void *userdata
    ) noexcept
    {
        // Store for later information about the current state of dirpath
        if (ice::string::back(dirpath) != L'/')
        {
            ice::string::push_back(dirpath, L'/');
        }

        ice::u32 const size_dirpath = ice::string::size(dirpath);
        ice::string::push_back(dirpath, L'*');

        WIN32_FIND_DATA direntry;
        HANDLE const handle = FindFirstFileW(
            ice::string::begin(dirpath),
            &direntry
        );
        ice::string::pop_back(dirpath);

        bool traverse_success = false;
        if (handle != INVALID_HANDLE_VALUE)
        {
            traverse_success = true;
            do
            {

                // We cast the value to a regular pointer so WString will use 'strlen' to get the final length.
                if (direntry.cFileName[0] == '.' && (direntry.cFileName[1] == '.' || direntry.cFileName[1] == '\0'))
                {
                    continue;
                }

                ice::native_file::EntityType const type = (direntry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0
                    ? EntityType::Directory
                    : EntityType::File;

                // Append the entry name to the path
                ice::native_file::FilePath const entry_name = (ice::wchar const*)direntry.cFileName;
                ice::string::push_back(dirpath, entry_name);

                // Call the callback for the next entry encountered...
                ice::native_file::TraverseAction const action = callback(basepath, dirpath, type, userdata);
                if (action == TraverseAction::Break)
                {
                    break;
                }

                // Enter the next directory recursively
                if (type == EntityType::Directory && action != TraverseAction::SkipSubDir) // If is dir
                {
                    traverse_success = traverse_directories_internal(basepath, dirpath, callback, userdata);
                    if (traverse_success == false)
                    {
                        break;
                    }
                }

                // Rollback the directory string to the base value
                ice::string::resize(dirpath, size_dirpath);

            } while (FindNextFileW(handle, &direntry) != FALSE);
            FindClose(handle);
        }
        return traverse_success;
    }

    bool traverse_directories(
        ice::native_file::FilePath starting_dir,
        ice::native_file::TraversePathCallback callback,
        void* userdata
    ) noexcept
    {
        ice::StackAllocator_1024 temp_alloc;
        ice::native_file::HeapFilePath dirpath{ temp_alloc };
        ice::string::reserve(dirpath, 256 * 2); // 512 bytes for paths
        ice::string::push_back(dirpath, starting_dir);
        return traverse_directories_internal(dirpath, dirpath, callback, userdata);
    }

    void path_from_string(
        ice::native_file::HeapFilePath& out_filepath,
        ice::String path_string
    ) noexcept
    {
        ice::string::clear(out_filepath);
        ice::utf8_to_wide_append(path_string, out_filepath);
    }

    void path_to_string(
        ice::native_file::FilePath path,
        ice::HeapString<>& out_string
    ) noexcept
    {
        ice::string::clear(out_string);
        ice::wide_to_utf8_append(path, out_string);
    }

    void path_join_string(
        ice::native_file::HeapFilePath& path,
        ice::String string
    ) noexcept
    {
        // TODO: Think if maybe moving this to a different function is possible?
        if (ice::string::any(path) && ice::string::back(path) != L'/' && ice::string::back(path) != L'\\')
        {
            if (ice::string::front(string) != '/' && ice::string::front(string) != '\\')
            {
                ice::string::push_back(path, L'/');
            }
        }
        ice::utf8_to_wide_append(string, path);
    }

#elif ISP_UNIX

    inline auto translate_flags(ice::native_file::FileOpenFlags flags) noexcept -> int
    {
        int result = O_RDONLY;
#if 0 // TODO: Not supported yet
        if (ice::has_any(flags, FileOpenFlags::Asynchronous))
        {
            result = O_ASYNC;
        }
#endif
        return result;
    }

    bool exists_file(ice::native_file::FilePath path) noexcept
    {
        struct stat file_stats;
        return stat(ice::string::begin(path), &file_stats) == 0 && file_stats.st_size > 0;
    }

    auto open_file(
        ice::native_file::FilePath path,
        ice::native_file::FileOpenFlags flags /*= FileOpenFlags::ReadOnly*/
    ) noexcept -> ice::native_file::File
    {
        ice::native_file::File result;
        if constexpr (ice::build::current_platform == ice::build::System::Android)
        {
            result = ice::native_file::File{
                open(ice::string::begin(path), translate_flags(flags))
            };
        }
        else if constexpr (ice::build::current_platform == ice::build::System::WebApp)
        {
            result = ice::native_file::File{
                open(ice::string::begin(path), translate_flags(flags))
            };
        }
        else
        {
            ICE_ASSERT_CORE(false); // TODO: None-Android async loading
        }
        return result;
    }

    auto sizeof_file(ice::native_file::File const& native_file) noexcept -> ice::usize
    {
        struct stat file_stats;
        fstat(native_file.native(), &file_stats);
        return { static_cast<ice::usize::base_type>(file_stats.st_size) };
    }

    auto sizeof_file(ice::native_file::FilePath path) noexcept -> ice::usize
    {
        struct stat file_stats;
        stat(ice::string::begin(path), &file_stats);
        return { static_cast<ice::usize::base_type>(file_stats.st_size) };
    }

    auto read_file(
        ice::native_file::File const& native_file,
        ice::usize requested_read_size,
        ice::Memory memory
    ) noexcept -> ice::usize
    {
        return read_file(native_file, 0_B, requested_read_size, memory);
    }

    auto read_file(
        ice::native_file::File const& native_file,
        ice::usize requested_read_offset,
        ice::usize requested_read_size,
        ice::Memory memory
    ) noexcept -> ice::usize
    {
        IPT_ZONE_SCOPED;
        ICE_ASSERT_CORE(memory.size >= requested_read_size);
        ice::isize::base_type bytes_read = pread(
            native_file.native(),
            memory.location,
            requested_read_size.value,
            requested_read_offset.value
        );
        if (bytes_read < 0)
        {
            bytes_read = 0;
        }
        return { static_cast<ice::usize::base_type>(bytes_read) };
    }

    bool traverse_directories_internal(
        ice::native_file::FilePath basepath,
        ice::native_file::HeapFilePath& dirpath,
        ice::native_file::TraversePathCallback callback,
        void* userdata
    ) noexcept
    {
        bool traverse_success = false;

        DIR* const directory = opendir(ice::string::begin(dirpath));
        if (directory != nullptr)
        {
            // Just opening the dir is already a success for us
            traverse_success = true;

            // Store for later information about the current state of dirpath
            if (ice::string::back(dirpath) != '/')
            {
                ice::string::push_back(dirpath, '/');
            }
            ice::ucount const size_dirpath = ice::string::size(dirpath);

            while (dirent const* const entry = readdir(directory))
            {
                // Skip 'parent' and 'self' special paths
                if (entry->d_name[0] == '.' && (entry->d_name[1] == '.' || entry->d_name[1] == '\0'))
                {
                    continue;
                }

                // Skip anything not beeing a file or directory
                if (entry->d_type != DT_DIR && entry->d_type != DT_REG)
                {
                    continue;
                }

                ice::native_file::EntityType const type = (entry->d_type == DT_DIR)
                    ? EntityType::Directory
                    : EntityType::File;

                // Append the entry name to the path
                ice::ucount const size_name = ice::ucount(strlen(entry->d_name));
                ice::string::push_back(dirpath, ice::native_file::FilePath{ entry->d_name, size_name });

                // Call the callback for the next entry encountered...
                ice::native_file::TraverseAction const action = callback(basepath, dirpath, type, userdata);
                if (action == TraverseAction::Break)
                {
                    break;
                }

                // Enter the next directory recursively
                if (type == EntityType::Directory && action != TraverseAction::SkipSubDir) // If is dir
                {
                    traverse_success = traverse_directories_internal(basepath, dirpath, callback, userdata);
                    if (traverse_success == false)
                    {
                        break;
                    }
                }

                // Rollback the directory string to the base value
                ice::string::resize(dirpath, size_dirpath);
            }

            closedir(directory);
        }

        return traverse_success;
    }

    bool traverse_directories(
        ice::native_file::FilePath starting_dir,
        ice::native_file::TraversePathCallback callback,
        void* userdata
    ) noexcept
    {
        ice::StackAllocator_1024 temp_alloc;
        ice::native_file::HeapFilePath dirpath{ temp_alloc };
        ice::string::reserve(dirpath, 256 * 2); // 512 bytes for paths
        ice::string::push_back(dirpath, starting_dir);
        return traverse_directories_internal(dirpath, dirpath, callback, userdata);
    }

    void path_from_string(
        ice::String path_string,
        ice::native_file::HeapFilePath& out_filepath
    ) noexcept
    {
        out_filepath = path_string;
    }

    void path_to_string(
        ice::native_file::FilePath path,
        ice::HeapString<>& out_string
    ) noexcept
    {
        out_string = path;
    }

    void path_join_string(
        ice::native_file::HeapFilePath& path,
        ice::String string
    ) noexcept
    {
        ice::path::join(path, string);
    }

#else
#error Not Implemented
#endif

} // namespace ice::native_file
