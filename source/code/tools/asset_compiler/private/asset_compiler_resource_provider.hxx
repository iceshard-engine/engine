/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/resource.hxx>
#include <ice/resource_provider.hxx>
#include <ice/native_file.hxx>
#include <ice/task.hxx>
#include <ice/uri.hxx>

class AssetCompilerResource final : public ice::Resource
{
public:
    AssetCompilerResource(
        ice::Allocator& alloc,
        ice::native_file::File file,
        ice::String filepath
    ) noexcept;
    ~AssetCompilerResource() noexcept override;

    auto uri() const noexcept -> ice::URI const& override;
    auto flags() const noexcept -> ice::ResourceFlags override;

    auto name() const noexcept -> ice::String override;
    auto origin() const noexcept -> ice::String override;

    auto file() const noexcept -> ice::native_file::File const& { return _handle; }

    ice::u32 idx;

private:
    ice::Allocator& _allocator;
    ice::native_file::File _handle;
    ice::Memory _metadata;
    ice::String _path;
    ice::URI _uri;
};

class AssetCompilerResourceProvider final : public ice::ResourceProvider
{
public:
    AssetCompilerResourceProvider(ice::Allocator& alloc, ice::Span<ice::String const> files) noexcept;
    ~AssetCompilerResourceProvider() noexcept override;

    auto schemeid() const noexcept -> ice::StringID override { return ice::Scheme_File; }

    auto collect(
        ice::Array<ice::Resource const*>& out_changes
    ) noexcept -> ice::ucount override;

    auto refresh(
        ice::Array<ice::Resource const*>& out_changes
    ) noexcept -> ice::ResourceProviderResult override;

    auto find_resource(
        ice::URI const& uri
    ) const noexcept -> ice::Resource const* override;

    auto access_loose_resource(
        ice::Resource const* resource
    ) const noexcept -> ice::LooseResource const* override;

    void unload_resource(
        ice::Allocator& alloc,
        ice::Resource const* resource,
        ice::Memory memory
    ) noexcept override;

    auto load_resource(
        ice::Resource const* resource,
        ice::String fragment
    ) noexcept -> ice::TaskExpected<ice::Data> override;

    auto resolve_relative_resource(
        ice::URI const& relative_uri,
        ice::Resource const* root_resource
    ) const noexcept -> ice::Resource const* override;

private:
    ice::Allocator& _allocator;
    ice::Array<AssetCompilerResource*> _resources;
    ice::Array<ice::Memory> _data;
};
