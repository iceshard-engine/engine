/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/string.hxx>
#include <ice/heap_string.hxx>
#include <ice/log_formatters.hxx>

namespace ice
{

    struct PathString
    {
        template<ice::concepts::SupportedCharType CharT>
        static constexpr ice::BasicString<CharT> Separator_Dot;
        template<ice::concepts::SupportedCharType CharT>
        static constexpr ice::BasicString<CharT> Separator_Drive;
        template<ice::concepts::SupportedCharType CharT>
        static constexpr ice::BasicString<CharT> Separator_Directory;

        template<> constexpr ice::BasicString<char> Separator_Dot<char> = ".";
        template<> constexpr ice::BasicString<char> Separator_Drive<char> = ":";
        template<> constexpr ice::BasicString<char> Separator_Directory<char> = "\\/";
        template<> constexpr ice::BasicString<ice::wchar> Separator_Dot<ice::wchar> = L".";
        template<> constexpr ice::BasicString<ice::wchar> Separator_Drive<ice::wchar> = L":";
        template<> constexpr ice::BasicString<ice::wchar> Separator_Directory<ice::wchar> = L"\\/";

        template<typename Self>
        bool is_absolute(this Self const& self) noexcept
        {
            using CharType = ice::string::CharType<Self>;

            if constexpr (ice::build::is_windows)
            {
                if (self.size() >= 3_count)
                {
                    return self[1] == Separator_Drive<CharType>[0] && Separator_Directory<CharType>.find_first_of(self[2]) != ice::nindex_none;
                }
                return false;
            }
            else
            {
                return self.not_empty() && self.front() == Separator_Directory<CharType>[1];
            }
        }

        template<typename Self>
        bool is_relative(this Self const& self) noexcept
        {
            return self.is_absolute() == false;
        }
    };

    template<ice::concepts::SupportedCharType CharT>
    struct BasicPath : public ice::BasicString<CharT>, public ice::PathString
    {
        using BasicString<CharT>::BasicString;
        using BasicString<CharT>::operator std::basic_string_view<CharT>;

        constexpr BasicPath(ice::BasicString<CharT> str) noexcept
            : BasicString<CharT>{ str }
        { }
    };

    using Path = ice::BasicPath<char>;

} // namespace

namespace ice::path
{

    //! \return true If the path is just the root part. (ex. 'C:/' or '/')
    bool is_absolute_root(ice::String path) noexcept;

    //! \return The lenght of the path.
    auto length(ice::String path) noexcept -> ice::u32;

    //! \return The last extension part (with the dot character) or empty string if no extension was found.
    auto extension(ice::String path) noexcept -> ice::String;

    //! \return The characters after the last directory separator character. (including the file extension)
    auto filename(ice::String path) noexcept -> ice::String;

    //! \return The characters after the last directory separator character. (excluding the file extension)
    auto basename(ice::String path) noexcept -> ice::String;

    //! \return The characters before the last directory separator character.
    //! \note If the path is ending with a separator character, it will only strip that character. (ex: /path/ -> /path)
    auto directory(ice::String path) noexcept -> ice::String;

    //! \brief Joins two paths together if possible, the result is stored in the first first variable.
    //! \note If the second path is an absolute path, it returns it without changes.
    //! \note The resulting path may not be normalized.
    //! \returns The given path as a String value.
    auto join(ice::HeapString<>& path, ice::String right_path) noexcept -> ice::String;

    //! \brief Normalizes the given path in using simple rules.
    //! \note Replaces forward slashes with backslashes. (\\ -> /)
    //! \note Removes duplicate backslashes.
    //! \note Resolves (if possible) all backtracking path parts. (../../)
    //! \returns The given path as a String value.
    auto normalize(ice::HeapString<>& path) noexcept -> ice::String;

    //! \brief Replaces the filename of in the path. (including the extension)
    //! \returns The given path as a String value.
    auto replace_filename(ice::HeapString<>& path, ice::String filename) noexcept -> ice::String;

    //! \brief Replaces the extension (if any) in the path.
    //! \note Only replaces the last extension part.
    //! \note Removes the last extension part if a empty string is provided.
    //! \note Appends the given extension if no extension was found.
    //! \returns The given path as a String value.
    auto replace_extension(ice::HeapString<>& path, ice::String extension) noexcept -> ice::String;

    // Wider character implementations

    bool is_absolute(ice::WString path) noexcept;
    bool is_absolute_root(ice::WString path) noexcept;
    auto length(ice::WString path) noexcept -> ice::u32;
    auto extension(ice::WString path) noexcept -> ice::WString;
    auto filename(ice::WString path) noexcept -> ice::WString;
    auto basename(ice::WString path) noexcept -> ice::WString;
    auto directory(ice::WString path) noexcept -> ice::WString;

    auto join(ice::HeapString<ice::wchar>& path, ice::WString right_path) noexcept -> ice::WString;
    auto normalize(ice::HeapString<ice::wchar>& path) noexcept -> ice::WString;
    auto replace_filename(ice::HeapString<ice::wchar>& path, ice::WString filename) noexcept -> ice::WString;
    auto replace_extension(ice::HeapString<ice::wchar>& path, ice::WString extension) noexcept -> ice::WString;

} // namespace ice::path

template<typename CharType>
struct fmt::formatter<ice::BasicPath<CharType>> : public fmt::formatter<std::basic_string_view<CharType>>
{
    template<typename FormatContext>
    constexpr auto format(ice::BasicPath<CharType> value, FormatContext& ctx) const noexcept
    {
        return fmt::formatter<std::basic_string_view<CharType>>::format(value, ctx);
    }
};
