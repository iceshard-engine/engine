/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "resource_provider_filesystem.hxx"
#include "resource_provider_hailstorm.hxx"

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
        , _resources{ _named_allocator }
        , _resources_data{ _data_allocator }
        , _devui_widget{ create_filesystem_provider_devui(_named_allocator, _resources) }
    {
        ice::native_file::HeapFilePath base_path{ _named_allocator };
        for (ice::String path : paths)
        {
            ice::native_file::path_from_string(base_path, path);
            ice::path::normalize(base_path);
            ice::array::push_back(_base_paths, base_path);
        }
    }

    FileSystemResourceProvider::~FileSystemResourceProvider() noexcept
    {
        for (ice::Memory const& res_data : _resources_data)
        {
            _data_allocator.deallocate(res_data);
        }
        for (FileSystemResource* res_entry : _resources)
        {
            _named_allocator.destroy(res_entry);
        }
    }

    auto FileSystemResourceProvider::schemeid() const noexcept -> ice::StringID
    {
        return ice::Scheme_File;
    }

    void FileSystemResourceProvider::create_resource_from_file(
        ice::native_file::FilePath base_path,
        ice::native_file::FilePath file_path
    ) noexcept
    {
        // Early out for metadata files.
        if (ice::path::extension(file_path) == ISP_PATH_LITERAL(".isrm"))
        {
            return;
        }

        ice::StackAllocator_1024 temp_alloc;
        ice::native_file::FilePath const uribase = ice::path::directory(base_path);
        ice::native_file::FilePath const datafile = file_path;
        ice::native_file::HeapFilePath metafile{ temp_alloc };
        ice::string::reserve(metafile, 512);
        ice::string::push_back(metafile, file_path);
        ice::string::push_back(metafile, ISP_PATH_LITERAL(".isrm"));

        ice::FileSystemResource* const resource = create_resources_from_loose_files(
            _named_allocator,
            base_path,
            uribase,
            metafile,
            datafile
        );

        if (resource != nullptr)
        {
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
        }
    }

    struct FileSystemResourceProvider::TraverseResourceRequest
    {
        FileSystemResourceProvider& self;
        ice::native_file::FilePath base_path;
        std::atomic_uint32_t& remaining;
        ice::TaskScheduler* worker_thread;
        ice::TaskScheduler* final_thread;
    };

    auto FileSystemResourceProvider::traverse_async(
        ice::native_file::HeapFilePath dir_path,
        TraverseResourceRequest& request
    ) noexcept -> ice::Task<>
    {
        IPT_ZONE_SCOPED;
        ice::native_file::traverse_directories(dir_path, traverse_callback, &request);
        request.remaining -= 1;
        co_return;
    }

    auto FileSystemResourceProvider::create_resource_from_file_async(
        ice::native_file::FilePath base_path,
        ice::native_file::HeapFilePath file_path,
        TraverseResourceRequest& request
    ) noexcept -> ice::Task<>
    {
        // Early out for metadata files.
        if (ice::path::extension(file_path) == ISP_PATH_LITERAL(".isrm"))
        {
            request.remaining -= 1;
            co_return;
        }

        ice::StackAllocator_1024 temp_alloc;
        ice::native_file::FilePath const uribase = ice::path::directory(base_path);
        ice::native_file::FilePath const datafile = file_path;
        ice::native_file::HeapFilePath metafile{ temp_alloc };
        ice::string::reserve(metafile, 512);
        ice::string::push_back(metafile, file_path);
        ice::string::push_back(metafile, ISP_PATH_LITERAL(".isrm"));

        ice::FileSystemResource* const resource = create_resources_from_loose_files(
            _named_allocator,
            base_path,
            uribase,
            metafile,
            datafile
        );

        co_await *request.final_thread;

        resource->data_index = ice::array::count(_resources_data);
        ice::array::push_back(_resources_data, ice::Memory{});

        if (resource != nullptr)
        {

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
        }

        request.remaining -= 1;
    }

    auto FileSystemResourceProvider::traverse_callback(
        ice::native_file::FilePath,
        ice::native_file::FilePath path,
        ice::native_file::EntityType type,
        void* userdata
    ) noexcept -> ice::native_file::TraverseAction
    {
        TraverseResourceRequest* request = reinterpret_cast<TraverseResourceRequest*>(userdata);
        if (type == ice::native_file::EntityType::File)
        {
            if (request->worker_thread != nullptr)
            {
                request->remaining += 1;
                ice::Allocator& alloc = request->self._named_allocator;
                ice::schedule_task(
                    request->self.create_resource_from_file_async(request->base_path, { alloc, path }, *request),
                    *request->worker_thread
                );
            }
            else
            {
                request->self.create_resource_from_file(request->base_path, path);
            }
        }
        else if (type == ice::native_file::EntityType::Directory && request->worker_thread != nullptr)
        {
            request->remaining += 1;

            ice::Allocator& alloc = request->self._named_allocator;
            ice::schedule_task(
                request->self.traverse_async({ alloc, path }, *request),
                *request->worker_thread
            );

            // Since we now run sub-directories as separate tasks, we skip them here
            return ice::native_file::TraverseAction::SkipSubDir;
        }

        // Continue in synchronous scenario
        return ice::native_file::TraverseAction::Continue;
    }

    void FileSystemResourceProvider::initial_traverse() noexcept
    {
        ice::Array<TraverseResourceRequest, ContainerLogic::Complex> requests{ _named_allocator };
        ice::array::reserve(requests, ice::array::count(_base_paths));

        [[maybe_unused]]
        std::atomic_uint32_t remaining = 0;
        for (ice::native_file::FilePath base_path : _base_paths)
        {
            ice::array::push_back(requests, { *this, base_path, remaining, nullptr, nullptr });
            ice::native_file::traverse_directories(
                base_path, traverse_callback, &ice::array::back(requests)
            );
        }
    }

    void FileSystemResourceProvider::initial_traverse_mt() noexcept
    {
        ice::TaskQueue local_queue{ };
        ice::TaskScheduler local_sched{ local_queue };

        ice::Array<TraverseResourceRequest, ContainerLogic::Complex> requests{ _named_allocator };
        ice::array::reserve(requests, ice::array::count(_base_paths));

        std::atomic_uint32_t remaining = 0;

        // Traverse directories synchronously but create resources asynchronously.
        for (ice::native_file::FilePath base_path : _base_paths)
        {
            ice::array::push_back(requests, { *this, base_path, remaining, _scheduler, &local_sched });
            ice::native_file::traverse_directories(
                base_path, traverse_callback, &ice::array::back(requests)
            );
        }

        // Process all awaiting tasks
        while (remaining > 0)
        {
            local_queue.process_all();
        }
    }

    auto FileSystemResourceProvider::collect(
        ice::Array<ice::Resource const*>& out_changes
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

    auto FileSystemResourceProvider::refresh(
        ice::Array<ice::Resource const*>& out_changes
    ) noexcept -> ice::ResourceProviderResult
    {
        IPT_ZONE_SCOPED;
        if (ice::hashmap::empty(_resources))
        {
            if (_scheduler == nullptr)
            {
                initial_traverse();
            }
            else // if (_scheduler != nullptr)
            {
                initial_traverse_mt();
            }
            collect(out_changes);
        }
        return ResourceProviderResult::Success;
    }

    auto FileSystemResourceProvider::find_resource(
        ice::URI const& uri
    ) const noexcept -> ice::Resource const*
    {
        ICE_ASSERT(
            uri.scheme() == ice::stringid_hash(schemeid()),
            "Trying to find resource for URI that is not handled by this provider."
        );

        if (ice::string::any(uri.host()) && _virtual_hostname != uri.host())
        {
            return nullptr;
        }

        ice::FileSystemResource const* found_resource = nullptr;
        ice::u32 const origin_size = ice::string::size(uri.path());

        ice::HeapString<> predicted_path{ (ice::Allocator&) _named_allocator };
        for (ice::native_file::FilePath base_path : _base_paths)
        {
            ice::string::resize(predicted_path, 0);
            ice::string::reserve(predicted_path, origin_size + ice::string::size(base_path));
            ice::native_file::path_to_string(base_path, predicted_path);

            // Remove one directory if neccessary, because it's may be the common value of the base path and the uri path.
            // Note: This is because if a base path like 'dir/subdir' is provided the uri is created against 'dir/'
            //  While a base path like 'dir/subdir/' will create uris against 'dir/subdir/'
            if (ice::string::back(base_path) != '/')
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
        ice::Allocator&,
        ice::Resource const* resource,
        ice::Memory
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
