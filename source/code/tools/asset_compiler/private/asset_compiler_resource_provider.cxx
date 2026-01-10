/// Copyright 2024 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "asset_compiler_resource_provider.hxx"
#include <ice/heap_string.hxx>
#include <ice/resource_flags.hxx>

AssetCompilerResource::AssetCompilerResource(
    ice::Allocator& alloc,
    ice::native_file::File file,
    ice::String filepath
) noexcept
    : _allocator{ alloc }
    , _handle{ ice::move(file) }
    , _metadata{ }
    , _path{ filepath }
    , _uri{ ice::Scheme_File, _path }
{
    ice::native_file::HeapFilePath metapath{ _allocator };
    ice::native_file::path_from_string(metapath, _path);
    metapath.push_back(ISP_PATH_LITERAL(".isrm"));

    if (auto metafile = ice::native_file::open_file(metapath, ice::native_file::FileOpenFlags::Read); file)
    {
        _metadata = _allocator.allocate(ice::native_file::sizeof_file(metafile));
        if (ice::native_file::read_file(metafile, _metadata.size, _metadata) != _metadata.size)
        {
            _allocator.deallocate(ice::exchange(_metadata, {}));
        }
    }
}

AssetCompilerResource::~AssetCompilerResource() noexcept
{
    _allocator.deallocate(_metadata);
}

auto AssetCompilerResource::uri() const noexcept -> ice::URI const&
{
    return _uri;
}

auto AssetCompilerResource::flags() const noexcept -> ice::ResourceFlags
{
    return ice::ResourceFlags::None;
}

auto AssetCompilerResource::name() const noexcept -> ice::String
{
    // return ice::path::filename(_path);
    return ice::path::basename(_path);
}

auto AssetCompilerResource::origin() const noexcept -> ice::String
{
    return _path;
}

AssetCompilerResourceProvider::AssetCompilerResourceProvider(
    ice::Allocator& alloc,
    ice::Span<ice::String const> files
) noexcept
    : _allocator{ alloc }
    , _resources{ _allocator }
    , _data{ _allocator }
{
    ice::array::reserve(_resources, files.size().u32());

    ice::u32 idx = 0;
    for (ice::String file : files)
    {
        ice::native_file::HeapFilePath filepath{ _allocator };
        ice::native_file::path_from_string(filepath, file);
        ice::native_file::File filehandle = ice::native_file::open_file(filepath, ice::native_file::FileOpenFlags::Read);
        ICE_ASSERT(filehandle == true, "Couldn't open file {}", file);

        AssetCompilerResource* res = _allocator.create<AssetCompilerResource>(_allocator, ice::move(filehandle), file);
        res->idx = idx++;
        ice::array::push_back(_resources, res);
    }

    _data.resize(_resources.size());
    ice::memset(_data.begin(), '\0', _data.size().bytes().value);
}

AssetCompilerResourceProvider::~AssetCompilerResourceProvider() noexcept
{
    for (AssetCompilerResource* resource : _resources)
    {
        _allocator.destroy(resource);
    }

    for (ice::Memory data : _data)
    {
        _allocator.deallocate(data);
    }
}

auto AssetCompilerResourceProvider::collect(
    ice::Array<ice::Resource*>& out_changes
) noexcept -> ice::u32
{
    return 0;
}

auto AssetCompilerResourceProvider::refresh(
    ice::Array<ice::Resource*>& out_changes
) noexcept -> ice::ResourceProviderResult
{
    for (AssetCompilerResource* res : _resources)
    {
        ice::array::push_back(out_changes, res);
    }
    return ice::ResourceProviderResult::Success;
}

auto AssetCompilerResourceProvider::find_resource(
    ice::URI const& uri
) const noexcept -> ice::Resource*
{
    return nullptr;
}

auto AssetCompilerResourceProvider::access_loose_resource(
    ice::Resource const* resource
) const noexcept -> ice::LooseResource const*
{
    return nullptr;
}

void AssetCompilerResourceProvider::unload_resource(
    ice::Resource const* resource
) noexcept
{
    AssetCompilerResource const* ac_resource = dynamic_cast<AssetCompilerResource const*>(resource);
    _allocator.deallocate(ice::exchange(_data[ac_resource->idx], {}));
}

auto AssetCompilerResourceProvider::load_resource(
    ice::Resource const* resource,
    ice::String fragment
) noexcept -> ice::TaskExpected<ice::Data>
{
    using ice::operator""_B;

    AssetCompilerResource const* ac_resource = dynamic_cast<AssetCompilerResource const*>(resource);

    ice::usize const filesize = ice::native_file::sizeof_file(ac_resource->file());
    if (filesize == 0_B)
    {
        co_return {};
    }

    _data[ac_resource->idx] = _allocator.allocate(filesize);
    if (ice::native_file::read_file(ac_resource->file(), filesize, _data[ac_resource->idx]) != filesize)
    {
        _allocator.deallocate(ice::exchange(_data[ac_resource->idx], {}));
    }

    co_return ice::data_view(_data[ac_resource->idx]);
}

auto AssetCompilerResourceProvider::resolve_relative_resource(
    ice::URI const& relative_uri,
    ice::Resource const* root_resource
) const noexcept -> ice::Resource const*
{
    return nullptr;
}
