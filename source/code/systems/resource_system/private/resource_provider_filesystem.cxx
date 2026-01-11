/// Copyright 2023 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "resource_provider_filesystem.hxx"
#include "resource_provider_hailstorm.hxx"
#include "resource_filesystem_baked.hxx"
#include <ice/resource_filter.hxx>
#include <ice/config.hxx>

namespace ice
{

    FileSystemResourceProvider::FileSystemResourceProvider(
        ice::Allocator& alloc,
        ice::Span<ice::String const> const& paths,
        ice::TaskScheduler* scheduler,
        ice::native_aio::AIOPort aioport,
        ice::String virtual_hostname
    ) noexcept
        : _named_allocator{ alloc, "FileSystem" }
        , _data_allocator{ _named_allocator, "Data" }
        , _base_paths{ _named_allocator }
        , _scheduler{ scheduler }
        , _aioport{ aioport }
        , _virtual_hostname{ virtual_hostname }
        , _traverser{ *this }
        , _resources{ _named_allocator }
        , _resources_data{ _data_allocator }
        , _devui_widget{ create_filesystem_provider_devui(_named_allocator, _resources) }
    {
        ice::native_file::HeapFilePath base_path{ _named_allocator };
        for (ice::String path : paths)
        {
            ice::native_file::path_from_string(base_path, path);
            ice::path::normalize(base_path);
            _base_paths.push_back(base_path);
        }
    }

    FileSystemResourceProvider::~FileSystemResourceProvider() noexcept
    {
        for (ice::Memory const& res_data : _resources_data)
        {
            _data_allocator.deallocate(res_data);
        }
        for (ice::FileSystemResource* res_entry : _resources)
        {
            ice::destroy_resource_object(_named_allocator, res_entry);
        }
    }

    auto FileSystemResourceProvider::schemeid() const noexcept -> ice::StringID
    {
        return ice::Scheme_File;
    }

    auto FileSystemResourceProvider::filter_resource_uris(
        ice::ResourceFilter const& filter,
        ice::Array<ice::URI>& out_uris
    ) noexcept -> ice::TaskExpected<ice::u32>
    {
        ice::u32 collected = 0;
        for (ice::FileSystemResource const* resource : _resources)
        {
            if (filter.allows_resource(resource))
            {
                ice::Data metadata_data{};
                if (filter.requires_metadata())
                {
                    metadata_data = co_await load_resource(resource, "meta");

                    if (filter.filter_thread() != nullptr)
                    {
                        co_await *filter.filter_thread();
                    }

                    if (metadata_data.location == nullptr || metadata_data.size == 0_B)
                    {
                        continue;
                    }

                    ice::Config metadata{};
                    ice::Memory metadata_mem{};
                    if (reinterpret_cast<char const*>(metadata_data.location)[0] == '{')
                    {
                        metadata = ice::config::from_json(_named_allocator, ice::string_from_data<char>(metadata_data), metadata_mem);
                    }
                    else
                    {
                        metadata = ice::config::from_data(metadata_data);
                    }

                    if (filter.allows_metadata(metadata) == false)
                    {
                        _named_allocator.deallocate(metadata_mem);
                        continue;
                    }

                    _named_allocator.deallocate(metadata_mem);
                }

                out_uris.push_back(resource->uri());
                collected += 1;
            }
        }
        co_return collected;
    }

    auto FileSystemResourceProvider::collect(
        ice::Array<ice::Resource*>& out_changes
    ) noexcept -> ice::u32
    {
        IPT_ZONE_SCOPED;

        out_changes.reserve(out_changes.size() + ice::hashmap::count(_resources));
        for (auto* resource : _resources)
        {
            out_changes.push_back(resource);
        }
        return ice::hashmap::count(_resources);
    }

    auto FileSystemResourceProvider::refresh(
        ice::Array<ice::Resource*>& out_changes
    ) noexcept -> ice::ResourceProviderResult
    {
        IPT_ZONE_SCOPED;
        if (ice::hashmap::empty(_resources))
        {
            if (_scheduler == nullptr)
            {
                _traverser.initial_traverse(_base_paths);
            }
            else // if (_scheduler != nullptr)
            {
                _traverser.initial_traverse_mt(_base_paths, *_scheduler);
            }
            collect(out_changes);
        }
        return ResourceProviderResult::Success;
    }

    auto FileSystemResourceProvider::find_resource(
        ice::URI const& uri
    ) const noexcept -> ice::Resource*
    {
        ICE_ASSERT(
            uri.scheme() == ice::stringid_hash(schemeid()),
            "Trying to find resource for URI that is not handled by this provider."
        );

        if (uri.host().not_empty() && _virtual_hostname != uri.host())
        {
            return nullptr;
        }

        ice::FileSystemResource* found_resource = nullptr;
        ice::ncount const origin_size = uri.path().size();

        ice::HeapString<> predicted_path{ (ice::Allocator&) _named_allocator };
        for (ice::native_file::FilePath base_path : _base_paths)
        {
            predicted_path.resize(0);
            predicted_path.reserve(origin_size + base_path.size());
            ice::native_file::path_to_string(base_path, predicted_path);

            // Remove one directory if neccessary, because it's may be the common value of the base path and the uri path.
            // Note: This is because if a base path like 'dir/subdir' is provided the uri is created against 'dir/'
            //  While a base path like 'dir/subdir/' will create uris against 'dir/subdir/'
            if (base_path.back() != '/')
            {
                ice::path::join(predicted_path, "..");
            }
            ice::path::join(predicted_path, uri.path());
            ice::path::normalize(predicted_path);

            ice::u64 const resource_hash = ice::hash(ice::String{ predicted_path });
            found_resource = ice::hashmap::get(_resources, resource_hash, nullptr);
            if (found_resource != nullptr)
            {
                break;
            }
        }
        return found_resource;
    }

    auto FileSystemResourceProvider::access_loose_resource(
        ice::Resource const* resource
    ) const noexcept -> ice::LooseResource const*
    {
        return static_cast<ice::LooseFilesResource const*>(resource);
    }

    void FileSystemResourceProvider::unload_resource(
        ice::Resource const* resource
    ) noexcept
    {
        ice::FileSystemResource const* const filesys_res = static_cast<ice::FileSystemResource const*>(resource);
        _data_allocator.deallocate(std::exchange(_resources_data[filesys_res->data_index], {}));
    }

    auto FileSystemResourceProvider::load_resource(
        ice::Resource const* resource,
        ice::String fragment
    ) noexcept -> ice::TaskExpected<ice::Data, ice::ErrorCode>
    {
        ice::FileSystemResource const* const filesys_res = static_cast<ice::FileSystemResource const*>(resource);
        co_return co_await filesys_res->load_data(
            _data_allocator, _resources_data[filesys_res->data_index], fragment, _aioport
        );
    }

    auto FileSystemResourceProvider::resolve_relative_resource(
        ice::URI const& relative_uri,
        ice::Resource const* root_resource
    ) const noexcept -> ice::Resource const*
    {
        ice::ncount const origin_size = root_resource->origin().size();

        ice::HeapString<> predicted_path{ (ice::Allocator&) _named_allocator };
        predicted_path.reserve(origin_size + relative_uri.path().size());

        predicted_path = root_resource->origin().substr(
            0, origin_size - ice::path::filename(root_resource->name()).size()
        );

        ice::path::join(predicted_path, relative_uri.path());
        ice::path::normalize(predicted_path);

        ice::u64 const resource_hash = ice::hash(ice::String{ predicted_path });

        ice::FileSystemResource const* found_resource = ice::hashmap::get(_resources, resource_hash, nullptr);
        if (found_resource != nullptr)
        {
            return found_resource;
        }
        else
        {
            return nullptr;
        }
    }

#pragma region Implementation of: FileSystemTraverser

    auto FileSystemResourceProvider::allocator() noexcept -> ice::Allocator&
    {
        return _named_allocator;
    }

    auto FileSystemResourceProvider::create_baked_resource(
        ice::native_file::FilePath filepath
    ) noexcept -> ice::FileSystemResource*
    {
        if constexpr (false)
        {
            ice::HeapString<> uri_base{ _named_allocator };
            ice::string::push_format(uri_base, "file://{}/", _virtual_hostname);

            return create_resource_from_baked_file(_named_allocator, *this, uri_base, filepath);
        }
        else
        {
            return nullptr;
        }
    }

    auto FileSystemResourceProvider::create_loose_resource(
        ice::native_file::FilePath base_path,
        ice::native_file::FilePath uri_base_path,
        ice::native_file::FilePath meta_filepath,
        ice::native_file::FilePath data_filepath
    ) noexcept -> ice::FileSystemResource*
    {
        return ice::create_resources_from_loose_files(
            _named_allocator,
            *this,
            base_path,
            uri_base_path,
            meta_filepath,
            data_filepath
        );
    }

    auto FileSystemResourceProvider::register_resource(
        ice::FileSystemResource* resource
    ) noexcept -> ice::Result
    {
        resource->data_index = _resources_data.size().u32();
        _resources_data.push_back(ice::Memory{});

        ice::u64 const hash = ice::hash(resource->origin());
        ICE_ASSERT(
            ice::hashmap::has(_resources, hash) == false,
            "A resource cannot be a explicit resource AND part of another resource."
        );

        ice::hashmap::set(
            _resources,
            hash,
            resource
        );

        return S_Ok;
    }

    void FileSystemResourceProvider::destroy_resource(
        ice::FileSystemResource* resource
    ) noexcept
    {
        ice::destroy_resource_object(_named_allocator, resource);
    }

#pragma endregion

} // namespace ice

auto ice::create_resource_provider(
    ice::Allocator& alloc,
    ice::Span<ice::String const> paths,
    ice::TaskScheduler* scheduler,
    ice::native_aio::AIOPort aioport,
    ice::String virtual_hostname
) noexcept -> ice::UniquePtr<ice::ResourceProvider>
{
    return ice::make_unique<ice::FileSystemResourceProvider>(alloc, alloc, paths, scheduler, aioport, virtual_hostname);
}

auto ice::create_resource_provider_hailstorm(
    ice::Allocator& alloc,
    ice::String path,
    ice::native_aio::AIOPort aioport
) noexcept -> ice::UniquePtr<ice::ResourceProvider>
{
    return ice::make_unique<ice::HailStormResourceProvider>(alloc, alloc, path, aioport);
}
