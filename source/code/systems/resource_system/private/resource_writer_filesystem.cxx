/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "resource_writer_filesystem.hxx"
#include "resource_filesystem_writable.hxx"

namespace ice
{

    FileSystemResourceWriter::FileSystemResourceWriter(
        ice::Allocator& alloc,
        ice::String base_path,
        ice::TaskScheduler* scheduler,
        ice::native_aio::AIOPort aioport,
        ice::String virtual_hostname
    ) noexcept
        : _named_allocator{ alloc, "FileSystem" }
        , _data_allocator{ _named_allocator, "Data" }
        , _base_path{ _named_allocator }
        , _scheduler{ scheduler }
        , _aioport{ aioport }
        , _virtual_hostname{ virtual_hostname }
        , _traverser{ *this }
        , _resources{ _named_allocator }
        , _resources_data{ _data_allocator }
        , _devui_widget{ }
    {
        ice::native_file::path_from_string(_base_path, base_path);
    }

    FileSystemResourceWriter::~FileSystemResourceWriter() noexcept
    {
        for (ice::Memory const& res_data : _resources_data)
        {
            _data_allocator.deallocate(res_data);
        }
        for (WritableFileSystemResource* res_entry : _resources)
        {
            _named_allocator.destroy(res_entry);
        }
    }

    auto FileSystemResourceWriter::schemeid() const noexcept -> ice::StringID
    {
        return ice::Scheme_File;
    }

    auto FileSystemResourceWriter::collect(
        ice::Array<ice::Resource*>& out_changes
    ) noexcept -> ice::ucount
    {
        IPT_ZONE_SCOPED;

        ice::array::reserve(out_changes, ice::array::count(out_changes) +  ice::hashmap::count(_resources));
        for (auto* resource : _resources)
        {
            ice::array::push_back(out_changes, resource);
        }
        return ice::hashmap::count(_resources);
    }

    auto FileSystemResourceWriter::refresh(
        ice::Array<ice::Resource*>& out_changes
    ) noexcept -> ice::ResourceProviderResult
    {
        IPT_ZONE_SCOPED;
        if (ice::hashmap::empty(_resources))
        {
            if (_scheduler == nullptr)
            {
                _traverser.initial_traverse({&_base_path, 1});
            }
            else // if (_scheduler != nullptr)
            {
                _traverser.initial_traverse_mt({ &_base_path, 1 }, *_scheduler);
            }
            collect(out_changes);
        }
        return ResourceProviderResult::Success;
    }

    auto FileSystemResourceWriter::find_resource(
        ice::URI const& uri
    ) const noexcept -> ice::Resource*
    {
        ICE_ASSERT(
            uri.scheme() == ice::stringid_hash(schemeid()),
            "Trying to find resource for URI that is not handled by this provider."
        );

        if (ice::string::any(uri.host()) && _virtual_hostname != uri.host())
        {
            return nullptr;
        }

        ice::u32 const origin_size = ice::string::size(uri.path());

        ice::HeapString<> predicted_path{ (ice::Allocator&) _named_allocator };
        ice::string::resize(predicted_path, 0);
        ice::string::reserve(predicted_path, origin_size + ice::string::size(_base_path));
        ice::native_file::path_to_string(_base_path, predicted_path);

        // Remove one directory if neccessary, because it's may be the common value of the base path and the uri path.
        // Note: This is because if a base path like 'dir/subdir' is provided the uri is created against 'dir/'
        //  While a base path like 'dir/subdir/' will create uris against 'dir/subdir/'
        if (ice::string::back(_base_path) != '/')
        {
            ice::path::join(predicted_path, "..");
        }
        ice::path::join(predicted_path, uri.path());
        ice::path::normalize(predicted_path);

        ice::u64 const resource_hash = ice::hash(ice::String{ predicted_path });
        return ice::hashmap::get(_resources, resource_hash, nullptr);
    }

    auto FileSystemResourceWriter::access_loose_resource(
        ice::Resource const* resource
    ) const noexcept -> ice::LooseResource const*
    {
        return static_cast<ice::WritableFileResource const*>(resource);
    }

    void FileSystemResourceWriter::unload_resource(
        ice::Resource const* resource
    ) noexcept
    {
        ice::FileSystemResource const* const filesys_res = static_cast<ice::FileSystemResource const*>(resource);
        _data_allocator.deallocate(std::exchange(_resources_data[filesys_res->data_index], {}));
    }

    auto FileSystemResourceWriter::load_resource(
        ice::Resource const* resource,
        ice::String fragment
    ) noexcept -> ice::TaskExpected<ice::Data, ice::ErrorCode>
    {
        ice::FileSystemResource const* const filesys_res = static_cast<ice::FileSystemResource const*>(resource);
        co_return co_await filesys_res->load_data(
            _data_allocator, _resources_data[filesys_res->data_index], fragment, _aioport
        );
    }

    auto FileSystemResourceWriter::resolve_relative_resource(
        ice::URI const& relative_uri,
        ice::Resource const* root_resource
    ) const noexcept -> ice::Resource const*
    {
        ice::u32 const origin_size = ice::string::size(root_resource->origin());

        ice::HeapString<> predicted_path{ (ice::Allocator&) _named_allocator };
        ice::string::reserve(predicted_path, origin_size + ice::string::size(relative_uri.path()));

        predicted_path = ice::string::substr(
            root_resource->origin(),
            0,
            origin_size - ice::string::size(
                ice::path::filename(root_resource->name())
            )
        );

        ice::path::join(predicted_path, relative_uri.path());
        ice::path::normalize(predicted_path);

        ice::u64 const resource_hash = ice::hash(ice::String{ predicted_path });

        ice::WritableFileSystemResource const* found_resource = ice::hashmap::get(_resources, resource_hash, nullptr);
        if (found_resource != nullptr)
        {
            return found_resource;
        }
        else
        {
            return nullptr;
        }
    }

    auto FileSystemResourceWriter::create_resource(
        ice::URI const& uri,
        ice::ResourceCreationFlags flags
    ) noexcept -> ice::TaskExpected<ice::Resource*>
    {
        if (ice::Resource* existing = find_resource(uri); existing != nullptr)
        {
            co_return existing;
        }

        ice::ucount predicted_path_len = 0;
        ice::native_file::HeapFilePath predicted_metapath{ (ice::Allocator&)_named_allocator };

        ICE_ASSERT_CORE(flags != ResourceCreationFlags::Append); // TODO

        // TODO: move into a utility function
        {
            ice::u32 const origin_size = ice::string::size(uri.path());

            ice::string::resize(predicted_metapath, 0);
            ice::string::reserve(predicted_metapath, origin_size + ice::string::size(_base_path));
            ice::path::join(predicted_metapath, _base_path);

            // Remove one directory if neccessary, because it's may be the common value of the base path and the uri path.
            // Note: This is because if a base path like 'dir/subdir' is provided the uri is created against 'dir/'
            //  While a base path like 'dir/subdir/' will create uris against 'dir/subdir/'
            if (ice::string::back(_base_path) != '/')
            {
                ice::path::join(predicted_metapath, ISP_PATH_LITERAL(".."));
            }
            ice::native_file::path_join_string(predicted_metapath, uri.path());
            ice::path::normalize(predicted_metapath);

            // Metapath is the actuall file path + .isrm, so we just save the lenght before the appending
            //  to have access to both paths.
            predicted_path_len = ice::string::size(predicted_metapath);
            ice::string::push_back(predicted_metapath, ISP_PATH_LITERAL(".isrm"));
        }

        ice::FileSystemResource* new_resource = this->create_loose_resource(
            _base_path,
            ice::path::directory(_base_path),
            predicted_metapath,
            ice::string::substr(predicted_metapath, 0, predicted_path_len)
        );

        if (register_resource(new_resource) != S_Ok)
        {
            ice::destroy_resource_object(_named_allocator, new_resource);
        }

        co_return new_resource;
    }

    auto FileSystemResourceWriter::write_resource(
        ice::Resource const* resource,
        ice::Data data,
        ice::usize write_offset
    ) noexcept -> ice::Task<bool>
    {
        ice::WritableFileResource const* const writable = static_cast<ice::WritableFileResource const*>(resource);
        ice::usize const result = co_await writable->write_data(_data_allocator, data, write_offset, {}, _aioport);
        co_return result > 0_B;
    }

#pragma region Implementation of: FileSystemTraverser

    auto FileSystemResourceWriter::allocator() noexcept -> ice::Allocator&
    {
        return _named_allocator;
    }

    auto FileSystemResourceWriter::create_baked_resource(
        ice::native_file::FilePath filepath
    ) noexcept -> ice::FileSystemResource*
    {
        return nullptr;
    }

    auto FileSystemResourceWriter::create_loose_resource(
        ice::native_file::FilePath base_path,
        ice::native_file::FilePath uri_base_path,
        ice::native_file::FilePath meta_filepath,
        ice::native_file::FilePath data_filepath
    ) noexcept -> ice::FileSystemResource*
    {
        return ice::create_writable_resources_from_loose_files(
            _named_allocator,
            *this,
            base_path,
            uri_base_path,
            meta_filepath,
            data_filepath
        );
    }

    auto FileSystemResourceWriter::register_resource(
        ice::FileSystemResource* fs_resource
    ) noexcept -> ice::Result
    {
        ice::WritableFileSystemResource* const resource = static_cast<ice::WritableFileSystemResource*>(fs_resource);

        resource->data_index = ice::array::count(_resources_data);
        ice::array::push_back(_resources_data, ice::Memory{});

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

    void FileSystemResourceWriter::destroy_resource(
        ice::FileSystemResource* resource
    ) noexcept
    {
        _named_allocator.destroy(resource);
    }

#pragma endregion

    auto create_resource_writer(
        ice::Allocator& alloc,
        ice::String base_path,
        ice::TaskScheduler* scheduler,
        ice::native_aio::AIOPort aioport,
        ice::String virtual_hostname
    ) noexcept -> ice::UniquePtr<ice::ResourceWriter>
    {
        return ice::make_unique<ice::FileSystemResourceWriter>(alloc, alloc, base_path, scheduler, aioport, virtual_hostname);
    }

} // namespace ice
