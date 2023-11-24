/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/string_types.hxx>
#include <ice/string/string.hxx>

namespace ice::path
{

    //! \note On windows: starts with a drive letter, on linux: checks for starting backslash.
    //! \return true If the path is absolute.
    bool is_absolute(ice::String path) noexcept;

    //! \return The lenght of the path.
    auto length(ice::String path) noexcept -> ice::ucount;

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
    auto length(ice::WString path) noexcept -> ice::ucount;
    auto extension(ice::WString path) noexcept -> ice::WString;
    auto filename(ice::WString path) noexcept -> ice::WString;
    auto basename(ice::WString path) noexcept -> ice::WString;
    auto directory(ice::WString path) noexcept -> ice::WString;

    auto join(ice::HeapString<ice::wchar>& path, ice::WString right_path) noexcept -> ice::WString;
    auto normalize(ice::HeapString<ice::wchar>& path) noexcept -> ice::WString;
    auto replace_filename(ice::HeapString<ice::wchar>& path, ice::WString filename) noexcept -> ice::WString;
    auto replace_extension(ice::HeapString<ice::wchar>& path, ice::WString extension) noexcept -> ice::WString;

} // namespace ice::path
