#include <ice/memory/pointer_arithmetic.hxx>

#include "path_utils.hxx"

namespace ice::path
{

    bool is_absolute(ice::Utf8String path) noexcept
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

    auto filename(ice::Utf8String str) noexcept -> ice::Utf8String
    {
        auto const separator_pos = ice::string::find_last_of(str, separators_directory);
        return ice::string::substr(str, separator_pos == ice::string_npos ? 0 : separator_pos + 1);
    }

    auto directory(ice::Utf8String str) noexcept -> ice::Utf8String
    {
        auto const separator_pos = ice::string::find_last_of(str, separators_directory);
        return ice::string::substr(str, separator_pos == ice::string_npos ? ice::string::size(str) : 0, separator_pos);
    }

    auto normalize(ice::HeapString<char8_t>& path) noexcept -> ice::Utf8String
    {
        char8_t* it = ice::string::begin(path);
        char8_t const* const beg = ice::string::begin(path);
        //char* reminder = nullptr;
        char8_t const* const end = ice::string::end(path);

        while (it != end)
        {
            if (*it == u8'\\')
            {
                *it = u8'/';
            }

            it += 1;
        }

        it = ice::string::begin(path);

        ice::u32 const begin = ice::string::find_first_of(path, u8':');
        if (begin != ice::string_npos)
        {
            it += begin + 1;
        }

        char8_t* copy_to = it;
        while (it != end)
        {
            if (*it == u8'.' && *(it + 1) == u8'.')
            {
                copy_to -= 1; // move to the previous slash '/'
                copy_to -= (copy_to != beg); // Move past the shash if we are not at the path begining

                while (*copy_to != u8'/' && copy_to != beg)
                {
                    copy_to -= 1;
                }

                it += 2; // Move past the two dots
            }

            if (copy_to != it)
            {
                *copy_to = *it;
            }

            it += 1;
            copy_to += 1;
        }

        ice::string::resize(path, ice::memory::ptr_distance(ice::string::begin(path), copy_to));

        return path;
    }

    auto join(ice::HeapString<char8_t>& left, ice::Utf8String right) noexcept -> ice::Utf8String
    {
        // This one was taken from MS's std::filesystem implementation.
        if (is_absolute(right))
        {
            return left.operator=(right);
        }

        if (auto last_char = ice::string::back(left); last_char != u8'/' && last_char != u8'.')
        {
            if (last_char == u8'\\')
            {
                ice::string::pop_back(left);
            }
            ice::string::push_back(left, u8'/');
        }

        if (right != u8".")
        {
            ice::string::push_back(left, right);
        }

        return left;
    }

    auto replace_filename(ice::HeapString<char8_t>& str, ice::Utf8String name) noexcept -> ice::Utf8String
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

#if ISP_WINDOWS

    bool is_absolute(ice::WString path) noexcept
    {
        if constexpr (ice::build::is_windows)
        {
            if (path.size() >= 3)
            {
                return path[1] == L':' && ice::string::find_first_of(Constant_DirectorySeparators, path[2]) != ice::string_npos;
            }
            return false;
        }
        else
        {
            return false;
        }
    }

    auto directory(ice::WString path) noexcept -> ice::WString
    {
        auto const separator_pos = ice::string::find_last_of(path, Constant_DirectorySeparators);
        return ice::string::substr(path, separator_pos == ice::string_npos ? ice::string::size(path) : 0, separator_pos);
    }

    auto join(ice::HeapString<wchar_t>& left, ice::WString right) noexcept -> ice::WString
    {
        // This one was taken from MS's std::filesystem implementation.
        if (is_absolute(right))
        {
            return left.operator=(right);
        }

        if (auto last_char = ice::string::back(left); last_char != '/' && last_char != '.')
        {
            if (last_char == '\\')
            {
                ice::string::pop_back(left);
            }
            ice::string::push_back(left, L'/');
        }

        if (right != L".")
        {
            ice::string::push_back(left, right);
        }

        return left;
    }

#endif

} // namespace ice::path
