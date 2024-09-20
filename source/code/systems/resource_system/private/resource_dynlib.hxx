/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/resource.hxx>
#include <ice/os/windows.hxx>
#include <ice/native_file.hxx>
#include <ice/string/heap_string.hxx>
#include <ice/uri.hxx>

namespace ice
{

    class Resource_DynLib final : public ice::Resource
    {
    public:
        Resource_DynLib(
            ice::HeapString<> origin_path,
            ice::String origin_name
        ) noexcept;

        ~Resource_DynLib() noexcept override = default;

        auto uri() const noexcept -> ice::URI const& override;
        auto flags() const noexcept -> ice::ResourceFlags override;

        auto name() const noexcept -> ice::String override;
        auto origin() const noexcept -> ice::String override;

    private:
        ice::HeapString<> _origin_path;
        ice::String _origin_name;

        ice::URI _uri;
    };

    auto create_dynlib_resource_from_path(
        ice::Allocator& alloc,
        ice::native_file::FilePath file_path
    ) noexcept -> ice::Resource*;

} // namespace ice
