/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/string_types.hxx>
#include <ice/string/string.hxx>

namespace ice::path
{

    bool is_absolute(ice::String path) noexcept;

    auto extension(ice::String path) noexcept -> ice::String;
    auto filename(ice::String path) noexcept -> ice::String;
    auto directory(ice::String path) noexcept -> ice::String;

    auto join(ice::HeapString<>& path, ice::String right_path) noexcept -> ice::String;
    auto normalize(ice::HeapString<>& path) noexcept -> ice::String;

    auto replace_filename(ice::HeapString<>& path, ice::String filename) noexcept -> ice::String;
    auto replace_extension(ice::HeapString<>& path, ice::String extension) noexcept -> ice::String;

    namespace win32
    {

        bool is_absolute(ice::WString path) noexcept;

        auto extension(ice::WString path) noexcept -> ice::WString;
        auto filename(ice::WString path) noexcept -> ice::WString;
        auto directory(ice::WString path) noexcept -> ice::WString;

        auto join(ice::HeapString<ice::wchar>& path, ice::WString right_path) noexcept -> ice::WString;
        auto normalize(ice::HeapString<ice::wchar>& path) noexcept -> ice::WString;

        auto replace_filename(ice::HeapString<ice::wchar>& path, ice::WString filename) noexcept -> ice::WString;
        auto replace_extension(ice::HeapString<ice::wchar>& path, ice::WString extension) noexcept -> ice::WString;

    } // namespace win32

} // namespace ice::path
