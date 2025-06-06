/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "resource_dynlib.hxx"
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
        , _uri{ ice::Scheme_Dynlib, ice::path::basename(_origin_name) }
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

    auto create_dynlib_resource_from_path(
        ice::Allocator& alloc,
        ice::ResourceProvider& provider,
        ice::native_file::FilePath dll_path
    ) noexcept -> ice::Resource*
    {
        ice::Resource* result = nullptr;

        if (ice::native_file::exists_file(dll_path))
        {
            ice::HeapString<> data_file_path{ alloc };
            ice::native_file::path_to_string(dll_path, data_file_path);

            // Need to get the string first before moving the HeapString path.
            ice::String const utf8_origin_name = ice::path::filename(data_file_path);
            result = ice::create_resource_object<ice::Resource_DynLib>(
                alloc,
                provider,
                ice::move(data_file_path),
                utf8_origin_name
            );
        }

        return result;
    }

} // namespace ice
