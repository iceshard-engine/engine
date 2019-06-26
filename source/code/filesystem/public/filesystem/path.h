#pragma once
#include <filesystem/filesystem_api.h>
#include <filesystem/details.h>

namespace fs
{
    namespace path
    {
        FILESYSTEM_API std::string name(std::string val);
        FILESYSTEM_API std::string name_no_ext(std::string val);

        FILESYSTEM_API std::string extension(std::string val);
        FILESYSTEM_API std::string extension_long(std::string val);
    }
}
