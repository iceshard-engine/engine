/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/resource_tracker.hxx>
#include <ice/resource_provider.hxx>
#include <ice/resource_writer.hxx>
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

#include "resource_internal.hxx"

namespace ice
{

    static constexpr ice::ErrorCode E_ResourceLoadNeeded{ "E.4300:Resources:Resource needs loading." };

    struct ResourceLoadTransaction : ice::TaskAwaitableBase
    {
        ice::ResourceHandle const& resource;
        ice::TaskTransaction& transaction;
        ice::u32 awaiter_index;

        ResourceLoadTransaction(ice::ResourceHandle const& handle, ice::TaskTransaction& transaction) noexcept
            : ice::TaskAwaitableBase{ }
            , resource{ handle }
            , transaction{ internal_ptr(resource)->start_transaction(transaction) }
            , awaiter_index{ ice::u32_max }
        {
        }

        ~ResourceLoadTransaction() noexcept
        {
            internal_ptr(resource)->finish_transaction(transaction);
        }

        inline bool await_ready() noexcept
        {
            ResourceStatus const status = internal_status(resource);
            // If loaded of load failed we return immediately because nothing else can be done.
            if (status == ResourceStatus::Loaded || status == ResourceStatus::Invalid)
            {
                return true;
            }

            awaiter_index = transaction.awaiters.fetch_add(1, std::memory_order_relaxed);
            return awaiter_index == 0;
        }

        inline bool await_suspend(std::coroutine_handle<> coro) noexcept
        {
            std::atomic_thread_fence(std::memory_order_acquire);

            // Check if the status is now updated
            ResourceStatus const status = internal_status(resource);

            // Another check after we are added as an awaiter to see if maybe the transaction already finished.
            if (status == ResourceStatus::Loaded || status == ResourceStatus::Invalid)
            {
                transaction.awaiters.fetch_sub(1, std::memory_order_relaxed);
                awaiter_index = ice::u32_max;
                return false;
            }

            _coro = coro;
            ice::linked_queue::push(transaction.queue, this);
            return true;
        }

        inline auto await_resume() const noexcept -> ice::Expected<ice::Data>
        {
            std::atomic_thread_fence(std::memory_order_acquire);

            // Check if the status is now updated
            ResourceStatus const status = internal_status(resource);

            if (awaiter_index == 0 && status == ResourceStatus::Available)
            {
                return E_ResourceLoadNeeded;
            }

            return internal_data(resource);
        }

        inline void finalize(ice::ResourceStatus status) noexcept
        {
            ice::internal_set_status(resource, status);

            // Ensure resource changes are visible after this point
            std::atomic_thread_fence(std::memory_order_release);

            ice::u32 awaiting = transaction.awaiters.load(std::memory_order_relaxed);

            // Loop as long as we did not reach the number of requests awaiting processing.
            ice::u32 requests_processed = 1; // Includes "us"
            while(requests_processed < awaiting)
            {
                for (ice::TaskAwaitableBase* awaitable : ice::linked_queue::consume(transaction.queue))
                {
                    // Resume to coroutine
                    awaitable->_coro.resume();
                    requests_processed += 1;
                }

                awaiting = transaction.awaiters.load(std::memory_order_relaxed);
            }

            transaction.awaiters.store(0, std::memory_order_relaxed);
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

        auto attach_writer(
            ice::UniquePtr<ice::ResourceWriter> provider
        ) noexcept -> ice::ResourceWriter* override;

        void sync_resources() noexcept override;

        auto find_resource(
            ice::URI const& resoure_uri,
            ice::ResourceFlags flags = ice::ResourceFlags::None
        ) const noexcept -> ice::ResourceHandle override;

        auto find_resource_relative(
            ice::URI const& uri,
            ice::ResourceHandle const& resource_handle
        ) const noexcept -> ice::ResourceHandle override;


        auto set_resource(
            ice::URI const& uri,
            ice::ResourceHandle const& resource_handle
        ) noexcept -> ice::Task<ice::ResourceResult> override;

        auto load_resource(
            ice::ResourceHandle const& resource_handle
        ) noexcept -> ice::Task<ice::ResourceResult> override;

        auto release_resource(
            ice::ResourceHandle const& resource_handle
        ) noexcept -> ice::Task<ice::ResourceResult> override;

        auto unload_resource(
            ice::ResourceHandle const& resource_handle
        ) noexcept -> ice::Task<ice::ResourceResult> override;


    public:
        auto create_resource(
            ice::URI const& uri
        ) noexcept -> ice::TaskExpected<ice::ResourceHandle> override;

        auto write_resource(
            ice::URI const& uri,
            ice::Data data,
            ice::usize write_offset = 0_B
        ) noexcept -> ice::Task<bool> override;

        auto write_resource(
            ice::ResourceHandle const& handle,
            ice::Data data,
            ice::usize write_offset = 0_B
        ) noexcept -> ice::Task<bool> override;

    public:
        class DevUI;

    protected:
        void sync_provider(
            ice::Array<ice::Resource*>& out_resources,
            ice::ResourceProvider& provider
        ) noexcept;

        auto find_resource_by_urn(
            ice::URI const& resoure_uri,
            ice::ResourceFlags flags
        ) const noexcept -> ice::ResourceHandle;

        auto find_resource_by_uri(
            ice::URI const& resoure_uri,
            ice::ResourceFlags flags
        ) const noexcept -> ice::ResourceHandle;

        bool find_resource_and_provider(
            ice::URI const& resource_uri,
            ice::ResourceProvider const*& provider,
            ice::Resource const*& resource
        ) const noexcept;

    private:
        ice::Allocator& _allocator;
        ice::ProxyAllocator _allocator_data;
        ice::ResourceTrackerCreateInfo _info;

        ice::HashMap<ice::Resource*> _resources;
        ice::HashMap<ice::UniquePtr<ice::ResourceProvider>, ContainerLogic::Complex> _resource_providers;
        ice::HashMap<ice::UniquePtr<ice::ResourceWriter>, ContainerLogic::Complex> _resource_writers;

        ice::UniquePtr<ice::DevUIWidget> _devui_widget;
    };

    auto create_tracker_devui(
        ice::Allocator& alloc,
        ice::ResourceTrackerImplementation& tracker
    ) noexcept -> ice::UniquePtr<ice::DevUIWidget>;

} // namespace ice
