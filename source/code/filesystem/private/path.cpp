#include <filesystem/path.h>

#include <string>

std::string fs::path::name(std::string val)
{
    auto begin = val.find_last_of('/');

    if (begin != std::string::npos)
        begin += 1;
    return val.substr(begin);
}

std::string fs::path::name_no_ext(std::string val)
{
    auto begin = val.find_last_of('/');
    auto end = val.find_last_of('.');

    if (begin != std::string::npos)
    {
        begin += 1;
    }
    else
    {
        begin = 0;
    }

    if (end != std::string::npos)
        end = end - begin;
    return val.substr(begin, end);
}

std::string fs::path::extension(std::string val)
{
    auto begin = val.find_last_of('.');

    if (begin == std::string::npos)
    {
        return "";
    }

    return val.substr(begin + 1);
}

std::string fs::path::extension_long(std::string val)
{
    auto begin = val.find_last_of('.');

    if (begin == std::string::npos)
    {
        return "";
    }

    return val.substr(begin + 1);
}