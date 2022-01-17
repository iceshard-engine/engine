#include <ice/resource_tracker.hxx>
#include <ice/resource_provider.hxx>
#include <ice/resource.hxx>
#include <ice/collections.hxx>
#include <ice/uri.hxx>
#include <ice/pod/hash.hxx>
#include <ice/task.hxx>
#include <ice/task_thread.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/task_sync_wait.hxx>
#include <ice/log_tag.hxx>

namespace ice
{

    static constexpr ice::LogTagDefinition LogTag_ResourceSystem = ice::create_log_tag(ice::LogTag::System, "Resources");

    struct ResourceHandle
    {
        ice::ResourceProvider* provider;
        ice::Resource_v2 const* resource;
        ice::Memory data;

        ice::ResourceStatus status;
        ice::u32 refcount;
        ice::u32 usecount;
    };

    auto resource_origin(ice::ResourceHandle const* handle) noexcept -> ice::Utf8String
    {
        return handle->resource->origin();
    }

    auto resource_object_DEPRECATED(ice::ResourceHandle const* handle) noexcept -> ice::Resource_v2 const*
    {
        return handle->resource;
    }

    class ResourceTracker_Impl final : public ice::ResourceTracker, public ice::TaskScheduler_v2
    {
    public:
        ResourceTracker_Impl(ice::Allocator& alloc, bool async) noexcept
            : _allocator{ alloc }
            , _handle_allocator{ _allocator }
            , _data_allocator{ _allocator }
            , _providers{ _allocator }
            , _tracked_handles{ _allocator }
            , _thread{ ice::make_unique_null<ice::TaskThread_v2>()}
            , _scheduler{ this }
        {
            if (async)
            {
                _thread = ice::create_task_thread_v2(_allocator);
                _scheduler = _thread.get();
            }
            ice::register_log_tag(LogTag_ResourceSystem);
        }

        ~ResourceTracker_Impl() noexcept
        {
            if (_thread != nullptr)
            {
                _thread->stop();
                _thread->join();
                _thread = nullptr;
            }

            for (auto const& entry : _tracked_handles)
            {
                entry.value->usecount -= 1;
                if (entry.value->usecount == 0)
                {
                    _data_allocator.deallocate(entry.value->data.location);
                    _handle_allocator.destroy(entry.value);
                }
            }
        }

        bool attach_provider(ice::ResourceProvider* provider) noexcept override
        {
            ICE_ASSERT(provider != nullptr, "Trying to attach a nullptr as provider.");

            ice::pod::Array<ice::Resource_v2 const*> temp_resources{ _allocator };

            ice::u64 const hash_value = ice::hash_from_ptr(provider);
            bool has_provider = ice::pod::hash::has(_providers, hash_value);
            if (has_provider == false)
            {
                ice::ResourceProviderResult const refresh_result = ice::sync_wait(provider->refresh());
                if (refresh_result != ice::ResourceProviderResult::Failure)
                {
                    has_provider = true;
                    ice::pod::hash::set(
                        _providers,
                        hash_value,
                        provider
                    );

                    provider->query_resources(temp_resources);
                    for (ice::Resource_v2 const* resource : temp_resources)
                    {
                        ice::Utf8String path = resource->uri().path;
                        ice::u64 const hash = ice::hash(path);
                        ice::ResourceHandle* handle = _handle_allocator.make<ice::ResourceHandle>(
                            ice::ResourceHandle{
                                .provider = provider,
                                .resource = resource,
                                .status = ice::ResourceStatus::Available,
                                .refcount = 0,
                                .usecount = 1,
                            }
                        );

                        ice::pod::hash::set(
                            _tracked_handles,
                            hash,
                            handle
                        );
                    }
                }
            }
            return has_provider;
        }

        bool detach_provider(ice::ResourceProvider* provider) noexcept override
        {
            ice::pod::hash::remove(_providers, ice::hash_from_ptr(provider));
            return true;
        }

        void refresh_providers() noexcept override
        {
            for (auto const& entry : _providers)
            {
                ice::ResourceProvider* provider = entry.value;

                ice::ResourceProviderResult const result = ice::sync_wait(provider->refresh());
                if (result == ice::ResourceProviderResult::Failure)
                {
                    ICE_LOG(
                        ice::LogSeverity::Error,
                        ice::LogTag_ResourceSystem, "Failed to update resource provider!"
                    );
                }
            }
        }

        void gather_resources_DEPRECATED(
            ice::pod::Array<ice::ResourceHandle*>& handles
        ) const noexcept override
        {
            for (auto const& entry : _tracked_handles)
            {
                ice::pod::array::push_back(handles, entry.value);
            }
        }

        auto find_resource(
            ice::URI const& uri,
            ice::ResourceFlags flags = ice::ResourceFlags::None
        ) const noexcept -> ice::ResourceHandle* override
        {
            ice::ResourceHandle* result = nullptr;

            ice::u64 const hash = ice::hash(uri.path);
            if (ice::pod::hash::has(_tracked_handles, hash))
            {
                ice::ResourceHandle* const handle = ice::pod::hash::get(
                    _tracked_handles,
                    hash,
                    nullptr
                );

                if (handle != nullptr && handle->resource != nullptr)
                {
                    ICE_ASSERT(handle->provider != nullptr, "Resource handle is missing associated provider object!");

                    // Save the result.
                    result = handle;
                }
            }

            return result;
        }

        auto find_resource_relative(
            ice::URI const& uri,
            ice::ResourceHandle* handle,
            ice::ResourceFlags flags
        ) const noexcept -> ice::ResourceHandle* override
        {
            ICE_ASSERT(handle != nullptr, "Trying to set resource from invalid handle!");
            ice::u64 const hash = ice::hash(uri.path);

            ice::ResourceHandle* result = nullptr;

            ice::URI const& resolved_uri = handle->provider->resolve_relative_uri(uri, handle->resource);
            if (resolved_uri.scheme != ice::uri_invalid.scheme)
            {
                result = this->find_resource(resolved_uri, flags);
            }
            return result;
        }

        //auto create_resource(
        //    ice::URI const& uri,
        //    ice::Metadata const& metadata,
        //    ice::Data data
        //) noexcept -> ice::Task<ice::ResourceResult> override
        //{
        //    co_return ice::ResourceResult{
        //        .resource_status = ice::ResourceStatus::Invalid
        //    };
        //}

        auto set_resource(
            ice::URI const& uri,
            ice::ResourceHandle* handle
        ) noexcept -> ice::Task<ice::ResourceResult> override
        {
            ICE_ASSERT(handle != nullptr, "Trying to set resource from invalid handle!");
            ice::u64 const hash = ice::hash(uri.path);

            ice::ResourceHandle* const replaced_handle = ice::pod::hash::get(_tracked_handles, hash, nullptr);
            ICE_ASSERT(replaced_handle != handle, "Cannot replace a resource with it's own value.");
            ICE_ASSERT(replaced_handle == nullptr || replaced_handle->refcount == 0, "Cannot replace a resource that is still in use!");

            if (replaced_handle == nullptr || replaced_handle->refcount == 0)
            {
                handle->usecount += 1;

                // Set the new handle to that resource name hash.
                ice::pod::hash::set(
                    _tracked_handles,
                    hash,
                    handle
                );

                if (replaced_handle != nullptr)
                {
                    replaced_handle->usecount -= 1;
                    if (replaced_handle->usecount == 0)
                    {
                        if (replaced_handle->status == ice::ResourceStatus::Loaded)
                        {
                            _data_allocator.deallocate(replaced_handle->data.location);
                        }
                        _handle_allocator.destroy(replaced_handle);
                    }
                }
            }

            co_return ice::ResourceResult{
                .resource_status = handle->status,
                .resource = handle->resource,
                .data = { }
            };
        }

        auto load_resource(
            ice::ResourceHandle* handle
        ) noexcept -> ice::Task<ice::ResourceResult> override
        {
            ICE_ASSERT(handle != nullptr, "Trying to load resource from invalid handle!");

            // Increase the refcount before doing anything.
            //  We require the user to handle thread-safe access for this part of the method so we are fine to do so.
            handle->refcount += 1;

            while (handle->status != ice::ResourceStatus::Loaded)
            {
                if (handle->status == ice::ResourceStatus::Available) // || status == ice::ResourceStatus::Unloaded)
                {
                    handle->status |= ice::ResourceStatus::Loading; // Add the 'Loading' flag so we don't enter this function twice.
                    handle->data = co_await handle->provider->load_resource(_data_allocator, handle->resource, *_scheduler);

                    if (handle->data.location != nullptr)
                    {
                        // We are now on the I/O thread, so we need to ensure that we are now properly behaving.
                        handle->status = ice::ResourceStatus::Loaded;
                    }
                    else
                    {
                        // A weird error occured the resource is not valid anymore!
                        handle->status = ice::ResourceStatus::Available | ice::ResourceStatus::Invalid;
                    }

                    std::atomic_thread_fence(std::memory_order::release);
                }
                else if (ice::has_flag(handle->status, ice::ResourceStatus::Loading))
                {
                    // Move the the I/O thread to wait for completion
                    while (ice::has_flag(handle->status, ice::ResourceStatus::Loading))
                    {
                        co_await *_thread;
                    }
                }
                else
                {
                    // We force exit here when we got errors.
                    break;
                }
            }

            if (handle->status != ice::ResourceStatus::Loaded)
            {
                co_return ice::ResourceResult{
                    .resource_status = handle->status,
                    .resource = handle->resource,
                    .data = { },
                };
            }

            co_return ice::ResourceResult{
                .resource_status = ice::ResourceStatus::Loaded,
                .resource = handle->resource,
                .data = handle->data,
            };
        }

        auto release_resource(
            ice::ResourceHandle* handle
        ) noexcept -> ice::Task<ice::ResourceResult> override
        {
            ICE_ASSERT(handle != nullptr, "Trying to release resource from invalid handle!");

            // Increase the refcount before doing anything.
            //  We require the user to handle thread-safe access for this part of the method so we are fine to do so.
            ICE_ASSERT(handle->refcount > 0, "Trying to release resource that is already in released state!");
            handle->refcount -= 1;

            co_return ice::ResourceResult{
                .resource_status = handle->status,
                .resource = handle->resource,
                .data = { .size = handle->data.size },
            };
        }

        auto unload_resource(
            ice::ResourceHandle* handle
        ) noexcept -> ice::Task<ice::ResourceResult> override
        {
            ICE_ASSERT(handle != nullptr, "Trying to unload resource from invalid handle!");

            // Increase the refcount before doing anything.
            //  We require the user to handle thread-safe access for this part of the method so we are fine to do so.
            ICE_ASSERT(handle->refcount > 0, "Trying to release resource that is already in released/unloaded state!");
            handle->refcount -= 1;

            if (handle->refcount == 0)
            {
                std::atomic_thread_fence(std::memory_order::acquire);

                ICE_ASSERT(
                    ice::has_flag(handle->status, ice::ResourceStatus::Loading),
                    "Trying to unload resource during loading!"
                );

                if (handle->status == ice::ResourceStatus::Loaded)
                {
                    _data_allocator.deallocate(handle->data.location);
                    handle->data = { };
                    handle->status = ice::ResourceStatus::Available;

                    std::atomic_thread_fence(std::memory_order::release);
                }
            }

            co_return ice::ResourceResult{
                .resource_status = handle->status,
                .resource = handle->resource,
                .data = {.size = handle->data.size },
            };
        }

        //auto update_resource(
        //    ice::Resource_v2 const* resource,
        //    ice::Metadata const* metadata,
        //    ice::Data data
        //) noexcept -> ice::Task<ice::ResourceResult> override
        //{
        //    co_return ice::ResourceResult{
        //        .resource_status = ice::ResourceStatus::Invalid
        //    };
        //}

        // Default behavior, just resume immediately and make a waited read.
        bool schedule(ice::TaskOperation_v2& operation) noexcept override
        {
            return false;
        }

    private:
        ice::Allocator& _allocator;
        ice::Allocator& _handle_allocator;
        ice::Allocator& _data_allocator;

        ice::pod::Hash<ice::ResourceProvider*> _providers;
        ice::pod::Hash<ice::ResourceHandle*> _tracked_handles;

        ice::UniquePtr<ice::TaskThread_v2> _thread;
        ice::TaskScheduler_v2* _scheduler;
    };

    auto create_resource_tracker(
        ice::Allocator& alloc,
        bool async
    ) noexcept -> ice::UniquePtr<ice::ResourceTracker>
    {
        return ice::make_unique<ice::ResourceTracker, ice::ResourceTracker_Impl>(alloc, alloc, async);
    }

} // namespace ice
