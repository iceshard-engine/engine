/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/resource_tracker.hxx>
#include <ice/resource_provider.hxx>
#include <ice/resource.hxx>
#include <ice/uri.hxx>
#include <ice/profiler.hxx>

#include <ice/string/string.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/log_tag.hxx>
#include <ice/log_formatters.hxx>

#include "native/native_aio.hxx"

namespace ice
{

    struct ResourceHandle
    {
        ResourceHandle(
            ice::Resource const* resource,
            ice::ResourceProvider* provider
        ) noexcept
            : resource{ resource }
            , provider{ provider }
            , status{ ResourceStatus::Available }
            , data{ }
            , refcount{ 0 }
        { }

        ResourceHandle(ResourceHandle const& other) noexcept
            : resource{ other.resource }
            , provider{ other.provider }
            , status{ other.status }
            , data{ other.data }
            , refcount{ other.refcount.load(std::memory_order_relaxed) }
        { }

        ice::Resource const* resource;
        ice::ResourceProvider* provider;
        ice::ResourceStatus status;
        ice::Memory data;
        std::atomic<ice::u32> refcount;
    };

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
    // Might need to be moved somewhere else?
    auto get_loose_resource(ice::ResourceHandle const* handle) noexcept -> ice::LooseResource const*
    {
        return handle->provider->access_loose_resource(handle->resource);
    }

    static_assert(std::atomic<std::coroutine_handle<>>::is_always_lock_free);

    class ResourceTrackerImplementation final : public ice::ResourceTracker
    {
    public:
        ResourceTrackerImplementation(
            ice::Allocator& alloc,
            ice::TaskScheduler& scheduler,
            ice::ResourceTrackerCreateInfo const& info
        ) noexcept;

        ~ResourceTrackerImplementation() noexcept;

        bool attach_provider(ice::ResourceProvider* provider) noexcept override;

        void sync_resources() noexcept override;

        auto find_resource(
            ice::URI const& resoure_uri,
            ice::ResourceFlags flags = ice::ResourceFlags::None
        ) const noexcept -> ice::ResourceHandle* override;

        auto find_resource_relative(
            ice::URI const& uri,
            ice::ResourceHandle* resource_handle
        ) const noexcept -> ice::ResourceHandle* override;


        auto set_resource(
            ice::URI const& uri,
            ice::ResourceHandle* resource_handle
        ) noexcept -> ice::Task<ice::ResourceResult> override;

        auto load_resource(
            ice::ResourceHandle* resource_handle
        ) noexcept -> ice::Task<ice::ResourceResult> override;

        auto release_resource(
            ice::ResourceHandle* resource_handle
        ) noexcept -> ice::Task<ice::ResourceResult> override;

        auto unload_resource(
            ice::ResourceHandle* resource_handle
        ) noexcept -> ice::Task<ice::ResourceResult> override;

    protected:
        auto find_resource_by_urn(
            ice::URI const& resoure_uri,
            ice::ResourceFlags flags
        ) const noexcept -> ice::ResourceHandle*;

        auto find_resource_by_uri(
            ice::URI const& resoure_uri,
            ice::ResourceFlags flags
        ) const noexcept -> ice::ResourceHandle*;

        bool find_resource_and_provider(
            ice::URI const& resource_uri,
            ice::ResourceProvider const*& provider,
            ice::Resource const*& resource
        ) const noexcept;

    private:
        ice::Allocator& _allocator;
        ice::TaskScheduler& _scheduler;
        ice::ResourceTrackerCreateInfo _info;

        ice::TaskQueue _io_queue;
        ice::UniquePtr<ice::NativeAIO> _io_thread_data;
        ice::UniquePtr<ice::TaskThread> _io_thread;

        ice::Array<ice::ResourceHandle, ContainerLogic::Complex> _handles;
        ice::HashMap<ice::ResourceHandle*> _resources;
        ice::HashMap<ice::ResourceProvider*> _resource_providers;
    };

    ResourceTrackerImplementation::ResourceTrackerImplementation(
        ice::Allocator& alloc,
        ice::TaskScheduler& scheduler,
        ice::ResourceTrackerCreateInfo const& info
    ) noexcept
        : ResourceTracker{ }
        , _allocator{ alloc }
        , _scheduler{ scheduler }
        , _info{ info }
        , _io_queue{ }
        , _io_thread_data{ }
        , _io_thread{ }
        , _handles{ _allocator }
        , _resources{ _allocator }
        , _resource_providers{ _allocator }
    {
        ICE_ASSERT(
            _info.predicted_resource_count > 0,
            "Invalid value ({}) provided for 'predicted_resource_count'. Value needs to be a positive integer."
        );

        ice::array::reserve(_handles, _info.predicted_resource_count);
        ice::hashmap::reserve(_resources, _info.predicted_resource_count);
        ice::hashmap::reserve(_resource_providers, 10);

        if (_info.io_dedicated_threads > 0)
        {
            _io_thread_data = ice::create_nativeio_thread_data(
                _allocator,
                _scheduler,
                _info.io_dedicated_threads
            );

            ice::TaskThreadInfo const io_thread_info{
                //.exclusive_queue = true, // ignored
                //.sort_by_priority = false, // ignored
                .custom_procedure = (ice::TaskThreadProcedure*)ice::nativeio_thread_procedure,
                .custom_procedure_userdata = _io_thread_data.get(),
                .debug_name = "ice.res_tracker",
            };
            _io_thread = ice::create_thread(_allocator, _io_queue, io_thread_info);
        }
    }

    ResourceTrackerImplementation::~ResourceTrackerImplementation() noexcept
    {
        for (ice::ResourceHandle* handle : _resources)
        {
            if (handle->refcount.load(std::memory_order_relaxed) > 0)
            {
                //IPT_MESSAGE_C("Encountered unreleased resource object during resource tracker destruction.", 0xEE99AA);
                _allocator.deallocate(handle->data);
            }
        }

        // Close thr thread first then the thread data.
        _io_thread.reset();
        _io_thread_data.reset();
    }

    bool ResourceTrackerImplementation::attach_provider(ice::ResourceProvider* provider) noexcept
    {
        ice::multi_hashmap::insert(
            _resource_providers,
            ice::hash(provider->schemeid()),
            provider
        );
        return true;
    }

    void ResourceTrackerImplementation::sync_resources() noexcept
    {
        IPT_ZONE_SCOPED;

        ice::Array<ice::Resource const*> temp_resources{ _allocator };
        for (ice::ResourceProvider* provider : _resource_providers)
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

            // Store all resource handles
            for (ice::Resource const* resource : temp_resources)
            {
                ice::array::push_back(
                    _handles,
                    ice::ResourceHandle {
                        resource,
                        provider
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
        if (resource_uri.scheme == ice::stringid_hash(ice::Scheme_URN))
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
        ice::TaskScheduler io_scheduler{ _io_queue };

        bool const load_task = resource_handle->refcount.fetch_add(1, std::memory_order_relaxed) == 0;
        if (load_task)
        {
            resource_handle->status = ResourceStatus::Loading;

            ice::Memory result{ };
            if (_info.io_dedicated_threads > 0)
            {
                result = co_await resource_handle->provider->load_resource(
                    _allocator,
                    resource_handle->resource,
                    io_scheduler,
                    _io_thread_data.get()
                );
            }
            else
            {
                // Load using provided scheduler. This allows us to load resources without using any multi threading at all if we wish to do so.
                co_await _scheduler.schedule(_info.flags_io_wait);

                result = co_await resource_handle->provider->load_resource(
                    _allocator,
                    resource_handle->resource,
                    io_scheduler,
                    nullptr /* If 'nullptr' it will load the resource synchronously */
                );
            }

            resource_handle->data = result;
            resource_handle->status = result.location == nullptr ? ResourceStatus::Invalid : ResourceStatus::Loaded;
            ICE_ASSERT(
                resource_handle->status != ResourceStatus::Invalid,
                "Failed loading '{}' resource",
                ice::resource_origin(resource_handle)
            );
        }
        else
        {
            // Busy wait to make sure we don't run into a data race on two threads trying to load same resource at the same time.
            while (resource_handle->status == ResourceStatus::Available)
            {
                co_await io_scheduler;
            }

            // Await the other thread to finish loading the data
            while (resource_handle->status == ResourceStatus::Loading)
            {
                co_await io_scheduler;
            }
        }

        co_await _scheduler.schedule(_info.flags_io_complete);

        //TracyMessageLC(resource_handle->resource->name()._data, 0xee2222ff);

        co_return ice::ResourceResult{
            .resource_status = resource_handle->status,
            .resource = resource_handle->resource,
            .data = ice::data_view(resource_handle->data),
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
        ice::Memory const data = resource_handle->data;
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
            _allocator.deallocate(data.location);

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
            resource_urn.scheme == ice::stringid_hash(Scheme_URN),
            "Invalid URI object. Expected resource name got [{}]",
            resource_urn.scheme
        );

        ice::ResourceHandle* result = nullptr;
        ice::u64 const hash_resouce = ice::hash(resource_urn.path);

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
        ice::u64 const hash_scheme = ice::hash(resource_uri.scheme);

        auto it = ice::multi_hashmap::find_first(_resource_providers, hash_scheme);
        while (it != nullptr && provider == nullptr)
        {
            if (resource = (*it)->find_resource(resource_uri); resource != nullptr)
            {
                provider = *it;
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
        ice::TaskScheduler& scheduler,
        ice::ResourceTrackerCreateInfo const& create_info
    ) noexcept -> ice::UniquePtr<ice::ResourceTracker>
    {
        return ice::make_unique<ice::ResourceTrackerImplementation>(alloc, alloc, scheduler, create_info);
    }

} // namespace ice
