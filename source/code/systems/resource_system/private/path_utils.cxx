#include <ice/memory.hxx>
#include "path_utils.hxx"

namespace ice::path
{

    bool is_absolute(ice::String path) noexcept
    {
        if constexpr (ice::build::is_windows)
        {
            if (path.size() >= 3)
            {
                return path[1] == ':' && ice::string::find_first_of(separators_directory, path[2]) != ice::string_npos;
            }
            return false;
        }
        else
        {
            return false;
        }
    }

    auto extension(ice::String str) noexcept -> ice::String
    {
        auto const separator_pos = ice::string::find_last_of(str, separators_extension);
        return ice::string::substr(str, separator_pos == ice::string_npos ? ice::string::size(str) : separator_pos);
    }

    auto filename(ice::String str) noexcept -> ice::String
    {
        auto const separator_pos = ice::string::find_last_of(str, separators_directory);
        return ice::string::substr(str, separator_pos == ice::string_npos ? 0 : separator_pos + 1);
    }

    auto directory(ice::String str) noexcept -> ice::String
    {
        auto const separator_pos = ice::string::find_last_of(str, separators_directory);
        return ice::string::substr(str, separator_pos == ice::string_npos ? ice::string::size(str) : 0, separator_pos);
    }

    auto normalize(ice::HeapString<> path) noexcept -> ice::String
    {
        char* it = ice::string::begin(path);
        char* reminder = nullptr;
        char const* end = ice::string::end(path);

        while(it != end)
        {
            if (*it == '\\')
            {
                *it = '/';
            }

            it += 1;
        }

        return path;
    }

    auto join(ice::HeapString<>& left, ice::String right) noexcept -> ice::String
    {
        // This one was taken from MS's std::filesystem implementation.
        if (is_absolute(right))
        {
            return left.operator=(right);
        }

        if (auto last_char = ice::string::back(left); last_char != '/')
        {
            if (last_char == '\\')
            {
                ice::string::pop_back(left);
            }
            ice::string::push_back(left, '/');
        }

        if (right != ".")
        {
            ice::string::push_back(left, right);
        }

        return left;
    }

    auto replace_filename(ice::HeapString<>& str, ice::String name) noexcept -> ice::String
    {
        auto const separator_pos = ice::string::find_last_of(str, separators_directory);
        if (separator_pos != ice::string_npos)
        {
            ice::string::resize(str, separator_pos + 1);
        }
        else
        {
            ice::string::clear(str);
        }

        if (!ice::string::empty(name))
        {
            ice::string::push_back(str, name);
        }

        return str;
    }

    auto replace_extension(ice::HeapString<>& str, ice::String extension) noexcept -> ice::String
    {
        auto const separator_pos = ice::string::find_last_of(str, separators_extension);
        if (separator_pos != ice::string_npos)
        {
            ice::string::resize(str, separator_pos + 1);
        }

        if (ice::string::empty(extension) == false)
        {
            if (ice::string::front(extension) != '.')
            {
                ice::string::push_back(str, '.');
            }
            ice::string::push_back(str, extension);
        }

        return str;
    }

} // namespace ice::path
