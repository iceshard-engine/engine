/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "resource_dynlib.hxx"
#include <ice/resource_meta.hxx>
#include <ice/resource_flags.hxx>
#include <ice/path_utils.hxx>

namespace ice
{

    Resource_DynLib::Resource_DynLib(
        ice::HeapString<> origin_path,
        ice::String origin_name
    ) noexcept
        : _origin_path{ ice::move(origin_path) }
        , _origin_name{ origin_name }
        , _uri{ ice::Scheme_Dynlib, _origin_name }
    {
    }

    auto Resource_DynLib::uri() const noexcept -> ice::URI const&
    {
        return _uri;
    }

    auto Resource_DynLib::flags() const noexcept -> ice::ResourceFlags
    {
        return ice::ResourceFlags::None;
    }

    auto Resource_DynLib::name() const noexcept -> ice::String
    {
        return _origin_name;
    }

    auto Resource_DynLib::origin() const noexcept -> ice::String
    {
        return _origin_path;
    }

    auto Resource_DynLib::metadata() const noexcept -> ice::Metadata const&
    {
        static ice::Metadata empty_meta;
        return empty_meta;
    }

    auto create_dynlib_resource_from_path(
        ice::Allocator& alloc,
        ice::native_fileio::FilePath dll_path
    ) noexcept -> ice::Resource*
    {
        ice::Resource* result = nullptr;

        if (ice::native_fileio::exists_file(dll_path))
        {
            ice::HeapString<> data_file_path{ alloc };
            ice::native_fileio::path_to_string(dll_path, data_file_path);

            // Need to get the string first before moving the HeapString path.
            ice::String const utf8_origin_name = ice::path::basename(data_file_path);
            result = alloc.create<ice::Resource_DynLib>(
                ice::move(data_file_path),
                utf8_origin_name
            );
        }

        return result;
    }

} // namespace ice
