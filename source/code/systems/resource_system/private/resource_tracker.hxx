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

namespace ice
{

    struct ResourceLoadContext : ice::TaskAwaitableBase
    {
        ice::ResourceHandle const& resource;
        ice::AtomicLinkedQueue<ice::TaskAwaitableBase> request_queue = {};
        ice::u16 request_load = 0;

        inline bool await_ready() noexcept
        {
            request_load = internal_ptr(resource)->_reqcount.fetch_add(1, std::memory_order_relaxed);
            return request_load <= 0 || internal_status(resource) == ResourceStatus::Loaded;
        }

        inline bool await_suspend(std::coroutine_handle<> coro) noexcept
        {
            // If count is below 0 then we are already "loaded" and can just continue
            if (internal_ptr(resource)->_reqcount.fetch_add(1, std::memory_order_relaxed) < 0)
            {
                return false;
            }

            // Else queue this load request and suspend
            ice::linked_queue::push(request_queue, this);
            return true;
        }

        inline bool await_resume() const noexcept// -> ice::Data
        {
            return request_load == 0;
            // We will get a pointer to the loaded data in the result part.
            // return *reinterpret_cast<ice::Data const*>(result.ptr);
        }

        inline void process_all() noexcept
        {
            // We get the number of queued requests and set int min as a replacement indicating that everything after
            //  this operation should just continue normally.
            ice::i32 const requests_awaiting = internal_ptr(resource)->_reqcount.exchange(ice::i16_min, std::memory_order_relaxed);
            ice::i32 requests_processed = 1;

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
        ) noexcept -> ice::Task<bool> override
        {
            co_return false;
        }

        auto write_resource(
            ice::ResourceHandle const& handle,
            ice::Data data,
            ice::usize write_offset = 0_B
        ) noexcept -> ice::Task<bool> override
        {
            co_return false;
        }

    public:
        class DevUI;

    protected:
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
