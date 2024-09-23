/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/resource_tracker.hxx>
#include <ice/resource_provider.hxx>
#include <ice/resource.hxx>
#include <ice/uri.hxx>
#include <ice/profiler.hxx>

#include <ice/string/string.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/log_tag.hxx>
#include <ice/log_formatters.hxx>
#include <ice/task_utils.hxx>
#include <ice/container/linked_queue.hxx>
#include <ice/path_utils.hxx>
#include <ice/devui_widget.hxx>

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
            , refcount{ 0 }
            , status{ ResourceStatus::Available }
            , data{ }
        { }

        ResourceHandle(ResourceHandle const& other) noexcept
            : resource{ other.resource }
            , provider{ other.provider }
            , refcount{ other.refcount.load(std::memory_order_relaxed) }
            , status{ other.status }
            , data{ other.data }
        { }

        ice::Resource const* resource;
        ice::ResourceProvider* provider;
        std::atomic<ice::u32> refcount;

        ice::ResourceStatus status;
        ice::Data data;
    };

    struct ResourceLoadContext : ice::TaskAwaitableBase
    {
        ice::ResourceHandle& resource;
        std::atomic<ice::i32> request_count = 0;
        ice::AtomicLinkedQueue<ice::TaskAwaitableBase> request_queue = {};
        bool request_load = false;

        inline bool await_ready() noexcept
        {
            request_load = resource.refcount.fetch_add(1, std::memory_order_relaxed) == 0;
            return request_load || resource.status == ResourceStatus::Loaded;
        }

        inline bool await_suspend(std::coroutine_handle<> coro) noexcept
        {
            // If count is below 0 then we are already "loaded" and can just continue
            if (request_count.fetch_add(1, std::memory_order_relaxed) < 0)
            {
                return false;
            }

            // Else queue this load request and suspend
            ice::linked_queue::push(request_queue, this);
            return true;
        }

        inline bool await_resume() const noexcept// -> ice::Data
        {
            return request_load;
            // We will get a pointer to the loaded data in the result part.
            // return *reinterpret_cast<ice::Data const*>(result.ptr);
        }

        inline void process_all() noexcept
        {
            // We get the number of queued requests and set int min as a replacement indicating that everything after
            //  this operation should just continue normally.
            ice::i32 const requests_awaiting = request_count.exchange(ice::i32_min, std::memory_order_relaxed);
            ice::i32 requests_processed = 0;

            // Loop as long as we did not reach the number of requests awaiting processing.
            while(requests_processed < requests_awaiting)
            {
                for (ice::TaskAwaitableBase* awaitable : ice::linked_queue::consume(request_queue))
                {
                    // Resume to coroutine
                    awaitable->_coro.resume();
                    requests_processed += 1;
                }
            }
        }
    };

    class ResourceTrackerImplementation final : public ice::ResourceTracker
    {
    public:
        ResourceTrackerImplementation(
            ice::Allocator& alloc,
            ice::ResourceTrackerCreateInfo const& info
        ) noexcept;

        ~ResourceTrackerImplementation() noexcept;

        auto attach_provider(
            ice::UniquePtr<ice::ResourceProvider> provider
        ) noexcept -> ice::ResourceProvider* override;

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

        class DevUI;

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
        ice::ProxyAllocator _allocator_handles;
        ice::ProxyAllocator _allocator_data;
        ice::ResourceTrackerCreateInfo _info;

        ice::Array<ice::ResourceHandle, ContainerLogic::Complex> _handles;
        ice::HashMap<ice::ResourceHandle*> _resources;
        ice::HashMap<ice::UniquePtr<ice::ResourceProvider>, ContainerLogic::Complex> _resource_providers;

        ice::UniquePtr<ice::DevUIWidget> _devui_widget;
    };

    auto create_tracker_devui(
        ice::Allocator& alloc,
        ice::ResourceTrackerImplementation& tracker
    ) noexcept -> ice::UniquePtr<ice::DevUIWidget>;

} // namespace ice
