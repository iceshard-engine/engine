/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/mem_utils.hxx>
#include <ice/string.hxx>
#include <ice/heap_string.hxx>
#include <ice/path_utils.hxx>

namespace ice::path
{

    namespace detail
    {

        template<typename CharType> static constexpr ice::BasicString<CharType> Separators_Dot;
        template<typename CharType> static constexpr ice::BasicString<CharType> Separators_Drive;
        template<typename CharType> static constexpr ice::BasicString<CharType> Separators_Directory;

        template<> constexpr ice::BasicString<char> Separators_Dot<char> = ".";
        template<> constexpr ice::BasicString<char> Separators_Drive<char> = ":";
        template<> constexpr ice::BasicString<char> Separators_Directory<char> = "\\/";
        template<> constexpr ice::BasicString<ice::wchar> Separators_Dot<ice::wchar> = L".";
        template<> constexpr ice::BasicString<ice::wchar> Separators_Drive<ice::wchar> = L":";
        template<> constexpr ice::BasicString<ice::wchar> Separators_Directory<ice::wchar> = L"\\/";

        template<typename CharType>
        constexpr bool is_absolute(ice::BasicString<CharType> path) noexcept
        {
            if constexpr (ice::build::is_windows)
            {
                if (path.size() >= 3)
                {
                    return path[1] == Separators_Drive<CharType>[0] && Separators_Directory<CharType>.find_first_of(path[2]) != ice::none_index;
                }
                return false;
            }
            else
            {
                return path.not_empty() && path.front() == Separators_Directory<CharType>[1];
            }
        }

        template<typename CharType>
        constexpr bool is_absolute_root(ice::BasicString<CharType> path) noexcept
        {
            if constexpr (ice::build::is_windows)
            {
                // Only support single-letter drives
                return path.size() == 3
                    && path[1] == Separators_Drive<CharType>[0]
                    && path.find_first_of(Separators_Directory<CharType>, path[2]) != ice::none_index;
            }
            else
            {
                return path.size() == 1 && path.front() == Separators_Directory<CharType>[1];
            }
        }

        template<typename CharType>
        constexpr auto extension(ice::BasicString<CharType> str) noexcept -> ice::BasicString<CharType>
        {
            ice::nindex const separator_pos = str.find_last_of(Separators_Dot<CharType>);
            return str.substr(separator_pos.value_or(str.size()));
        }

        template<typename CharType>
        constexpr auto filename(ice::BasicString<CharType> str) noexcept -> ice::BasicString<CharType>
        {
            ice::nindex const separator_pos = str.find_last_of(Separators_Directory<CharType>) + 1;
            return str.substr(separator_pos.value_or(0));
        }

        template<typename CharType>
        constexpr auto basename(ice::BasicString<CharType> str) noexcept -> ice::BasicString<CharType>
        {
            ice::BasicString<CharType> const name = filename(str);
            auto const extension_pos = name.find_last_of(Separators_Dot<CharType>);
            return name.substr(0, extension_pos);
        }

        template<typename CharType>
        constexpr auto directory(ice::BasicString<CharType> str) noexcept -> ice::BasicString<CharType>
        {
            ice::nindex const separator_pos = str.find_last_of(Separators_Directory<CharType>);
            if (separator_pos == ice::none_index)
            {
                return str.substr(str.size(), separator_pos);
            }

            if constexpr (ice::build::is_windows)
            {
                if (separator_pos > 0 && str[separator_pos - 1] == ':')
                {
                    // Keep the separator so we end up with C:/
                    return str.substr(0, separator_pos + 1);
                }
            }

            return str.substr(0, separator_pos);
        }

        template<typename CharType>
        auto join(ice::HeapString<CharType>& left, ice::BasicString<CharType> right) noexcept -> ice::BasicString<CharType>
        {
            // This one was taken from MS's std::filesystem implementation.
            if (is_absolute(right))
            {
                left = right;
                return left;
            }

            if (auto last_char = left.back(); last_char != Separators_Directory<CharType>[1])
            {
                if (last_char == Separators_Directory<CharType>[0])
                {
                    left.pop_back();
                }
                left.push_back(Separators_Directory<CharType>[1]);
            }

            if (right.front() == Separators_Directory<CharType>[1] || right.front() == Separators_Directory<CharType>[0])
            {
                right = right.substr(right.find_first_not_of(Separators_Directory<CharType>));
            }

            if (right != Separators_Dot<CharType>)
            {
                left.push_back(right);
            }

            return left;
        }

        template<typename CharType>
        auto normalize(ice::HeapString<CharType>& path) noexcept -> ice::BasicString<CharType>
        {
            bool const abs = detail::is_absolute<CharType>(path);
            CharType const* end = path.end();
            CharType* const beg = path.begin();
            if (beg == end)
            {
                return path;
            }

            // Replace separators to all be 'backslashes'
            CharType* it = beg;
            while (it != end)
            {
                if (*it == Separators_Directory<CharType>[0])
                {
                    *it = Separators_Directory<CharType>[1];
                }

                it += 1;
            }

            // Remove duplicated backslashes
            CharType* copy_to = it = beg;
            // it = beg + 1;
            while (it != end)
            {
                constexpr auto Separator = Separators_Directory<CharType>[1];
                while (*it == Separator && *(it + 1) == Separator)
                {
                    it += 1;
                }

                *copy_to = *it;
                copy_to += 1;
                it += 1;
            }

            // Set the new end iterator
            *copy_to = CharType{ 0 };
            end = copy_to;

            // Find starting position for next solve
            it = beg;

            ice::nindex const begin = path.find_first_of(Separators_Drive<CharType>);
            if (begin != ice::none_index)
            {
                it = it + begin + 1;
            }

            copy_to = it;
            while (it != end)
            {
                // Solve returning '..' paths
                if (*it == Separators_Dot<CharType>[0] && *(it + 1) == Separators_Dot<CharType>[0] && it > beg)
                {
                    copy_to -= 1; // move to the previous slash '/'
                    copy_to -= (copy_to != beg); // Move past the slash if we are not at the path begining

                    while (*copy_to != Separators_Directory<CharType>[1] && copy_to != beg)
                    {
                        copy_to -= 1;
                    }

                    it += 2; // Move past the two dots
                }

                if (copy_to == beg && abs == false)
                {
                    it += *it == Separators_Directory<CharType>[1];
                }

                if (copy_to != it)
                {
                    *copy_to = *it;
                }

                it += 1;
                copy_to += 1;
            }

            path.resize(copy_to - beg);
            return path;
        }

        template<typename CharType>
        auto replace_filename(ice::HeapString<CharType>& str, ice::BasicString<CharType> name) noexcept -> ice::BasicString<CharType>
        {
            auto const separator_pos = str.find_last_of(Separators_Directory<CharType>);
            if (separator_pos != ice::none_index)
            {
                str.resize(separator_pos + 1);
            }
            else
            {
                str.clear();
            }

            str.push_back(name);
            return str;
        }

        template<typename CharType>
        auto replace_extension(ice::HeapString<CharType>& str, ice::BasicString<CharType> extension) noexcept -> ice::BasicString<CharType>
        {
            auto const separator_pos = str.find_last_of(Separators_Dot<CharType>[0]);
            if (separator_pos != ice::none_index)
            {
                str.resize(separator_pos);
            }

            if (str.not_empty())
            {
                if (extension.front() != Separators_Dot<CharType>[0])
                {
                    str.push_back(Separators_Dot<CharType>[0]);
                }
                str.push_back(extension);
            }
            return str;
        }

    } // namespace detail

    bool is_absolute(ice::String path) noexcept { return detail::is_absolute(path); }
    bool is_absolute_root(ice::String path) noexcept { return detail::is_absolute_root(path); }
    auto length(ice::String path) noexcept -> ice::u32 { return path.size().u32(); }
    auto extension(ice::String path) noexcept -> ice::String { return detail::extension(path); }
    auto filename(ice::String path) noexcept -> ice::String { return detail::filename(path); }
    auto basename(ice::String path) noexcept -> ice::String { return detail::basename(path); }
    auto directory(ice::String path) noexcept -> ice::String { return detail::directory(path); }
    auto join(ice::HeapString<>& left, ice::String right) noexcept -> ice::String { return detail::join(left, right); }
    auto normalize(ice::HeapString<>& path) noexcept -> ice::String { return detail::normalize(path); }
    auto replace_filename(ice::HeapString<>& path, ice::String filename) noexcept -> ice::String { return detail::replace_filename(path, filename); }
    auto replace_extension(ice::HeapString<>& path, ice::String extension) noexcept -> ice::String { return detail::replace_extension(path, extension); }

    bool is_absolute(ice::WString path) noexcept { return detail::is_absolute(path); }
    bool is_absolute_root(ice::WString path) noexcept { return detail::is_absolute_root(path); }
    auto length(ice::WString path) noexcept -> ice::u32 { return path.size().u32(); }
    auto extension(ice::WString path) noexcept -> ice::WString { return detail::extension(path); }
    auto filename(ice::WString path) noexcept -> ice::WString { return detail::filename(path); }
    auto basename(ice::WString path) noexcept -> ice::WString { return detail::basename(path); }
    auto directory(ice::WString path) noexcept -> ice::WString { return detail::directory(path); }
    auto join(ice::HeapString<ice::wchar>& left, ice::WString right) noexcept -> ice::WString { return detail::join(left, right); }
    auto normalize(ice::HeapString<ice::wchar>& path) noexcept -> ice::WString { return detail::normalize(path); }
    auto replace_filename(ice::HeapString<ice::wchar>& path, ice::WString filename) noexcept -> ice::WString { return detail::replace_filename(path, filename); }
    auto replace_extension(ice::HeapString<ice::wchar>& path, ice::WString extension) noexcept -> ice::WString { return detail::replace_extension(path, extension); }

} // namespace ice::path
