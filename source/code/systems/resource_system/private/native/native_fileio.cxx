#include "native_fileio.hxx"
#include <ice/string/heap_string.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <ice/path_utils.hxx>

#if ISP_UNIX
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#endif

namespace ice::native_fileio
{

#if ISP_WINDOWS
#elif ISP_UNIX
    bool exists_file(
        ice::native_fileio::FilePath path
    ) noexcept
    {
        struct stat file_stats;
        return stat(ice::string::begin(path), &file_stats) == 0 && file_stats.st_size > 0;
    }

    auto file_open(
        ice::native_fileio::FilePath path
    ) noexcept -> ice::native_fileio::File
    {
        ice::native_fileio::File result;
        if constexpr (ice::build::current_platform == ice::build::System::Android)
        {
            result = ice::native_fileio::File{
                open(ice::string::begin(path), O_RDONLY)
            };
        }
        else
        {
            ICE_ASSERT_CORE(false); // TODO: None-Android async loading
        }
        return result;
    }

    auto sizeof_file(
        ice::native_fileio::File const& file
    ) noexcept -> ice::usize
    {
        struct stat file_stats;
        fstat(file.native(), &file_stats);
        return { static_cast<ice::usize::base_type>(file_stats.st_size) };
    }

    auto read_file(
        ice::native_fileio::File file,
        ice::usize requested_read_size,
        ice::Memory memory
    ) noexcept -> ice::usize
    {
        ICE_ASSERT_CORE(memory.size.value >= requested_read_size);
        ice::isize::base_type bytes_read = read(
            file.native(),
            memory.location,
            requested_read_size.value
        );
        if (bytes_read >= 0 && bytes_read)
        {
            bytes_read = 0;
        }
        return { static_cast<ice::usize::base_type>(bytes_read) };
    }

    bool traverse_directories_internal(
        ice::native_fileio::FilePath basepath,
        ice::native_fileio::HeapFilePath& dirpath,
        ice::native_fileio::TraversePathCallback callback,
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
            ice::string::push_back(dirpath, '/');
            ice::ucount const size_dirpath = ice::string::size(dirpath);

            while(dirent const* const entry = readdir(directory))
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

                ice::native_fileio::EntityType const type = (entry->d_type == DT_DIR)
                    ? EntityType::Directory
                    : EntityType::File;

                // Append the entry name to the path
                ice::ucount const size_name{ ice::ucount(entry->d_off) - ice::ucount(offsetof(dirent, d_name)) };
                ice::string::push_back(dirpath, ice::native_fileio::FilePath{ entry->d_name, size_name });

                // Call the callback for the next entry encountered...
                ice::native_fileio::TraverseAction const action = callback(basepath, dirpath, type, userdata);
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
        ice::native_fileio::FilePath starting_dir,
        ice::native_fileio::TraversePathCallback callback,
        void* userdata
    ) noexcept
    {
        ice::StackAllocator_1024 temp_alloc;
        ice::native_fileio::HeapFilePath dirpath{ temp_alloc };
        ice::string::reserve(dirpath, 256 * 3); // 768 bytes for paths
        ice::string::push_back(dirpath, starting_dir);
        return traverse_directories_internal(dirpath, dirpath, callback, userdata);
    }

    void path::from_string(
        ice::String path_string,
        ice::native_fileio::HeapFilePath& out_filepath
    ) noexcept
    {
        out_filepath = path_string;
    }

    void path::to_string(
        ice::native_fileio::FilePath path,
        ice::HeapString<>& out_string
    ) noexcept
    {
        out_string = path;
    }

    void path::join(
        ice::native_fileio::HeapFilePath& path,
        ice::String string
    ) noexcept
    {
        ice::path::join(path, string);
    }

    auto path::length(
        ice::native_fileio::FilePath path
    ) noexcept -> ice::ucount
    {
        return ice::string::size(path);
    }
#else
#error Not Implemented
#endif

} // namespace ice::native_fileio
