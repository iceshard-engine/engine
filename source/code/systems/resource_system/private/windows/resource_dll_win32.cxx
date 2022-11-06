/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "resource_dll_win32.hxx"
#include <ice/resource_meta.hxx>
#include <ice/resource_flags.hxx>

#if ISP_WINDOWS
#include "resource_utils_win32.hxx"

namespace ice
{

    Resource_DllsWin32::Resource_DllsWin32(
        ice::HeapString<> origin_path,
        ice::String origin_name
    ) noexcept
        : _origin_path{ ice::move(origin_path) }
        , _origin_name{ origin_name }
        , _uri{ ice::Scheme_Dynlib, _origin_name }
    {
    }

    auto Resource_DllsWin32::uri() const noexcept -> ice::URI const&
    {
        return _uri;
    }

    auto Resource_DllsWin32::flags() const noexcept -> ice::ResourceFlags
    {
        return ice::ResourceFlags::None;
    }

    auto Resource_DllsWin32::name() const noexcept -> ice::String
    {
        return _origin_name;
    }

    auto Resource_DllsWin32::origin() const noexcept -> ice::String
    {
        return _origin_path;
    }

    auto Resource_DllsWin32::metadata() const noexcept -> ice::Metadata const&
    {
        static ice::Metadata empty_meta;
        return empty_meta;
    }


    auto create_resource_from_dll_path(
        ice::Allocator& alloc,
        ice::WString dll_path
    ) noexcept -> ice::Resource*
    {
        ice::Resource* result = nullptr;

        DWORD const file_attribs = GetFileAttributesW(ice::string::begin(dll_path));
        if (file_attribs != INVALID_FILE_ATTRIBUTES)
        {
            ice::HeapString<> data_file_path{ alloc };
            win32::wide_to_utf8(dll_path, data_file_path);

            ice::String utf8_origin_name = ice::string::substr(
                data_file_path,
                ice::string::find_last_of(dll_path, L'/') + 1
            );

            result = alloc.create<ice::Resource_DllsWin32>(
                ice::move(data_file_path),
                utf8_origin_name
            );
        }

        return result;
    }

} // namespace ice

#endif
