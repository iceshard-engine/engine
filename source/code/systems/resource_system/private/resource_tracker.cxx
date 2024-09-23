/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "resource_tracker.hxx"

namespace ice
{

    auto resource_uri(ice::ResourceHandle const* handle) noexcept -> ice::URI const&
    {
        return handle->resource->uri();
    }

    auto resource_origin(ice::ResourceHandle const* handle) noexcept -> ice::String
    {
        return handle->resource->origin();
    }

    auto resource_path(ice::ResourceHandle const* handle) noexcept -> ice::String
    {
        return handle->resource->name();
    }

    auto resource_meta(ice::ResourceHandle const* handle, ice::Data& out_metadata) noexcept -> ice::Task<ice::Result>
    {
        out_metadata = co_await handle->provider->load_resource(handle->resource, "meta");
        co_return out_metadata.location == nullptr;
    }

    // Might need to be moved somewhere else?
    auto get_loose_resource(ice::ResourceHandle const* handle) noexcept -> ice::LooseResource const*
    {
        return handle->provider->access_loose_resource(handle->resource);
    }

    static_assert(std::atomic<std::coroutine_handle<>>::is_always_lock_free);

    ResourceTrackerImplementation::ResourceTrackerImplementation(
        ice::Allocator& alloc,
        ice::ResourceTrackerCreateInfo const& info
    ) noexcept
        : ResourceTracker{ }
        , _allocator{ alloc }
        , _allocator_handles{ _allocator, "Handles" }
        , _allocator_data{ _allocator, "Data" }
        , _info{ info }
        , _handles{ _allocator_handles }
        , _resources{ _allocator }
        , _resource_providers{ _allocator }
        , _devui_widget{ }
    {
        ICE_ASSERT(
            _info.predicted_resource_count > 0,
            "Invalid value ({}) provided for 'predicted_resource_count'. Value needs to be a positive integer."
        );

        ice::array::reserve(_handles, _info.predicted_resource_count);
        ice::hashmap::reserve(_resources, _info.predicted_resource_count);
        ice::hashmap::reserve(_resource_providers, 10);
    }

    ResourceTrackerImplementation::~ResourceTrackerImplementation() noexcept
    {
        for (ice::ResourceHandle* handle : _resources)
        {
            if (handle->refcount.load(std::memory_order_relaxed) > 0)
            {
                handle->provider->unload_resource(_allocator_data, handle->resource, {}/* REMOVE */);
                //IPT_MESSAGE_C("Encountered unreleased resource object during resource tracker destruction.", 0xEE99AA);
            }
        }
    }

    auto ResourceTrackerImplementation::attach_provider(
        ice::UniquePtr<ice::ResourceProvider> provider
    ) noexcept -> ice::ResourceProvider*
    {
        ice::ResourceProvider* const result = provider.get();
        ice::multi_hashmap::insert(
            _resource_providers,
            ice::hash(provider->schemeid()),
            ice::move(provider)
        );
        return result;
    }

    void ResourceTrackerImplementation::sync_resources() noexcept
    {
        IPT_ZONE_SCOPED;
        _devui_widget = ice::create_tracker_devui(_allocator, *this);

        ice::Array<ice::Resource const*> temp_resources{ _allocator };
        for (auto const& provider : _resource_providers)
        {
            ice::array::clear(temp_resources);

            ice::ResourceProviderResult const refresh_result = provider->refresh(temp_resources);
            if (refresh_result == ResourceProviderResult::Failure)
            {
                ICE_LOG(
                    ice::LogSeverity::Warning, ice::LogTag::Engine,
                    "Failed to refresh resource provider for scheme: {}",
                    ice::stringid_hint(provider->schemeid())
                );
                continue;
            }

            ice::ucount const new_count = ice::hashmap::count(_resources) + ice::array::count(temp_resources);
            ICE_ASSERT(
                new_count <= _info.predicted_resource_count,
                "Maximum resource capacity of {} entiries reached!",
                _info.predicted_resource_count
            );

            ice::hashmap::reserve(_resources, new_count);
            ice::array::reserve(_handles, new_count);

            // Store all resource handles
            IPT_ZONE_SCOPED_NAMED("create_hash_entries");
            for (ice::Resource const* resource : temp_resources)
            {
                ice::array::push_back(
                    _handles,
                    ice::ResourceHandle {
                        resource,
                        provider.get()
                    }
                );

                ice::multi_hashmap::insert(
                    _resources,
                    ice::hash(resource->name()),
                    ice::addressof(ice::array::back(_handles))
                );
            }
        }
    }

    auto ResourceTrackerImplementation::find_resource(
        ice::URI const& resource_uri,
        ice::ResourceFlags flags
    ) const noexcept -> ice::ResourceHandle*
    {
        if (resource_uri.scheme() == ice::stringid_hash(ice::Scheme_URN))
        {
            return find_resource_by_urn(resource_uri, flags);
        }
        else
        {
            return find_resource_by_uri(resource_uri, flags);
        }
    }

    auto ResourceTrackerImplementation::find_resource_relative(
        ice::URI const& uri,
        ice::ResourceHandle* handle
    ) const noexcept -> ice::ResourceHandle*
    {
        ICE_ASSERT(handle != nullptr, "Trying to set resource from invalid handle!");

        ice::ResourceHandle* result = nullptr;
        ice::Resource const* resource = handle->provider->resolve_relative_resource(uri, handle->resource);
        if (resource != nullptr)
        {
            result = this->find_resource(resource->uri(), resource->flags());
        }
        return result;
    }

    auto ResourceTrackerImplementation::set_resource(
        ice::URI const& uri,
        ice::ResourceHandle* resource_handle
    ) noexcept -> ice::Task<ice::ResourceResult>
    {
        co_return { .resource_status = ResourceStatus::Invalid };
    }

    auto ResourceTrackerImplementation::load_resource(
        ice::ResourceHandle* resource_handle
    ) noexcept -> ice::Task<ice::ResourceResult>
    {
        ice::ResourceLoadContext load_context{ .resource = *resource_handle };
        if (co_await load_context)
        {
            resource_handle->status = ResourceStatus::Loading;

            ice::Expected<ice::Data, ice::ErrorCode> const result
                = co_await resource_handle->provider->load_resource(resource_handle->resource, {});

            if (result.failed())
            {
                ICE_LOG(
                    LogSeverity::Error, LogTag::Engine,
                    "Failed to load resource {} with error: {}",
                    ice::resource_origin(resource_handle),
                    result.error()
                );
                resource_handle->status = ResourceStatus::Invalid;
            }
            else
            {
                resource_handle->data = result.value();
                resource_handle->status = ResourceStatus::Loaded;
            }

            // Ensure resource_handle changes are visible after this point
            std::atomic_thread_fence(std::memory_order_release);

            // Process all awaiting load contexts.
            // TODO: Schedule all of them instead?
            load_context.process_all();
        }
        else
        {
            ICE_ASSERT_CORE(resource_handle->status == ResourceStatus::Loaded);
        }

        co_return ice::ResourceResult{
            .resource_status = resource_handle->status,
            .resource = resource_handle->resource,
            .data = resource_handle->data,
        };
    }

    auto ResourceTrackerImplementation::release_resource(
        ice::ResourceHandle* resource_handle
    ) noexcept -> ice::Task<ice::ResourceResult>
    {
        ICE_ASSERT(resource_handle != nullptr, "Trying to release resource from invalid handle!");

        // Increase the refcount before doing anything.
        //  We require the user to handle thread-safe access for this part of the method so we are fine to do so.
        ice::u32 const last_count = resource_handle->refcount.fetch_sub(1, std::memory_order_relaxed);
        ICE_ASSERT(last_count > 0, "Trying to release resource that is already in released state!");

        co_return ice::ResourceResult{
            .resource_status = resource_handle->status,
            .resource = resource_handle->resource,
            .data = { .size = resource_handle->data.size },
        };
    }

    auto ResourceTrackerImplementation::unload_resource(
        ice::ResourceHandle* resource_handle
    ) noexcept -> ice::Task<ice::ResourceResult>
    {
        ICE_ASSERT(resource_handle != nullptr, "Trying to unload resource from invalid handle!");

        // Since we don't release data in the 'load_resource' method even if we would run into an unload/load situation
        //  we need to save the data information here before we decrease the refcount.
        //  Once decreased another request could already increase it and start loading new data and finish before
        //  we have a chance get the pointer to be released. (might happen... probably will)
        // ice::Memory const data = resource_handle->data;
        ice::ResourceStatus const last_status = resource_handle->status;
        ice::u32 const last_refcount = resource_handle->refcount.load(std::memory_order_relaxed);

        // Set the status to 'Available' is refcount == 1, don't decrease it yet
        // As if something will increase it we will just wait on the other thread until we reverse this operation.
        if (last_refcount == 1)
        {
            resource_handle->status = ResourceStatus::Available;
        }

        // Decrease the refcount before doing anything.
        ice::u32 const last_count = resource_handle->refcount.fetch_sub(1, std::memory_order_relaxed);
        ICE_ASSERT(last_count > 0, "Trying to release resource that is already in released/unloaded state!");

        if (last_count == 1)
        {
            // We can now safely release the current saved memory pointer.
            resource_handle->provider->unload_resource(
                _allocator_data, resource_handle->resource, {} /* REMOVE */
            );

            // We don't update the internal state nor the data member, as these will be considered invalid since refcount == 0
        }
        // If the initial refcount == 1 we need to revert the status change...
        else if (last_refcount == 1)
        {
            resource_handle->status = last_status;
        }

        co_return ice::ResourceResult{
            .resource_status = resource_handle->status,
            .resource = resource_handle->resource,
            .data = { .size = resource_handle->data.size },
        };
    }

    auto ResourceTrackerImplementation::find_resource_by_urn(
        ice::URI const& resource_urn,
        ice::ResourceFlags flags
    ) const noexcept -> ice::ResourceHandle*
    {
        ICE_ASSERT(
            resource_urn.scheme() == ice::stringid_hash(Scheme_URN),
            "Invalid URI object. Expected resource name got [{}]",
            resource_urn.scheme()
        );

        ice::ResourceHandle* result = nullptr;
        ice::u64 const hash_resouce = ice::hash(resource_urn.path());

        // Just grab the first for now
        auto it = ice::multi_hashmap::find_first(_resources, hash_resouce);
        if (it != nullptr)
        {
            result = it.value();
        }
        return result;
    }

    auto ResourceTrackerImplementation::find_resource_by_uri(
        ice::URI const& resource_uri,
        ice::ResourceFlags flags
    ) const noexcept -> ice::ResourceHandle*
    {
        ice::ResourceHandle* handle = nullptr;
        ice::Resource const* resource = nullptr;
        ice::ResourceProvider const* provider = nullptr;

        if (find_resource_and_provider(resource_uri, provider, resource) == false)
        {
            return nullptr;
        }

        ice::u64 const hash_resouce = ice::hash(resource->name());
        auto it = ice::multi_hashmap::find_first(_resources, hash_resouce);
        while (it != nullptr && handle == nullptr)
        {
            if ((*it)->resource == resource)
            {
                handle = *it;
            }
            else
            {
                it = ice::multi_hashmap::find_next(_resources, it);
            }
        }

        return handle;
    }

    bool ResourceTrackerImplementation::find_resource_and_provider(
        ice::URI const& resource_uri,
        ice::ResourceProvider const*& provider,
        ice::Resource const*& resource
    ) const noexcept
    {
        ice::u64 const hash_scheme = ice::hash(resource_uri.scheme());

        auto it = ice::multi_hashmap::find_first(_resource_providers, hash_scheme);
        while (it != nullptr && provider == nullptr)
        {
            if (resource = (*it)->find_resource(resource_uri); resource != nullptr)
            {
                provider = (*it).get();
            }
            else
            {
                it = ice::multi_hashmap::find_next(_resource_providers, it);
            }
        }

        return provider != nullptr;
    }

    auto create_resource_tracker(
        ice::Allocator& alloc,
        ice::ResourceTrackerCreateInfo const& create_info
    ) noexcept -> ice::UniquePtr<ice::ResourceTracker>
    {
        return ice::make_unique<ice::ResourceTrackerImplementation>(alloc, alloc, create_info);
    }

    auto resolve_dynlib_path(
        ice::ResourceTracker const& tracker,
        ice::Allocator& alloc,
        ice::String name
    ) noexcept -> ice::HeapString<>
    {
        // Try to reslove the name of the library with a dynlib URI.
        ice::URI const dynlib_uri{ ice::Scheme_Dynlib, name };
        if (ice::ResourceHandle* res = tracker.find_resource(dynlib_uri); res != nullptr)
        {
            return { alloc, ice::resource_origin(res) };
        }

        // ... next with a URN with the expected library file name.
        ice::HeapString<> result{ alloc, name };
        if (ice::path::extension(name) == "") // Check that we have already a full file name.
        {
            if constexpr (ice::build::is_windows)
            {
                ice::string::push_back(result, ".dll");
            }
            else
            {
                // On unix we expect the library to be prepended with 'lib' and appended with '.so'
                ice::string::clear(result);
                ice::string::push_back(result, "lib");
                ice::string::push_back(result, name);
                ice::string::push_back(result, ".so");
            }
        }

        ice::URI const urn_uri{ ice::Scheme_URN, name };
        if (ice::ResourceHandle* res = tracker.find_resource(dynlib_uri); res != nullptr)
        {
            return { alloc, ice::resource_origin(res) };
        }

        // ... and finally return the unresolved file name.
        return result;
    }

} // namespace ice
