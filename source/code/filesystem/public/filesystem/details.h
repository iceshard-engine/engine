#pragma once
#include <fstream>
#include <string>
#include <memory>

namespace fs
{
    class file;
    using file_ptr = std::unique_ptr<file>;

    namespace detail
    {
        using seekdir = std::ios_base::seekdir;
        using openmode = std::ios_base::openmode;

        using find_file_func_type = file_ptr(*)(std::string);
        using open_file_func_type = file_ptr(*)(std::string, openmode);
        using copy_file_func_type = file_ptr(*)(std::string, std::string);
        using move_file_func_type = file_ptr(*)(std::string, std::string);

        using delete_file_func_type = bool(*)(std::string);
    }

    // Import most used typedefs
    using seekdir = detail::seekdir;
    using openmode = detail::openmode;
    using ios = std::ios;
}
