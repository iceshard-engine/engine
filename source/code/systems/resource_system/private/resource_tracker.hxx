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

    class ResourceTrackerImplementation final : public ice::ResourceTracker
    {
    public:
        ResourceTrackerImplementation(
            ice::Allocator& alloc,
            ice::TaskScheduler& scheduler,
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
        ice::TaskScheduler& _scheduler;
        ice::ResourceTrackerCreateInfo _info;

        ice::TaskQueue _io_queue;
        ice::UniquePtr<ice::NativeAIO> _io_thread_data;
        ice::UniquePtr<ice::TaskThread> _io_thread;

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
