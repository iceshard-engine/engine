/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/shard.hxx>
#include <ice/mem_data.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/container_types.hxx>
#include <ice/heap_string.hxx>
#include <ice/resource_flags.hxx>
#include <ice/resource_handle.hxx>
#include <ice/resource_status.hxx>
#include <ice/uri.hxx>

#include <ice/task.hxx>
#include <ice/task_flags.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/task_expected.hxx>

namespace ice
{

    struct ResourceResult
    {
        ice::ResourceStatus resource_status;
        ice::ResourceHandle resource;
        ice::Data data;
    };

    struct ResourceTrackerCreateInfo
    {
        //! \brief The number of resources that should be tracked.
        //!
        //! \detail To avoid locking and complex mutli-threading logic this tracker pre-allocates a hashmap used to store resource handles.
        //!     This comes with a small up-front cost in memory but is generally neglible compared to the gains in simplicity and performance.
        //!     This value can be generous during development and can be toned down later. I
        //!     If you have trackers that update or provide new resources during the application lifetime, please increase the count for them.
        //!
        //! \note Setting this value is required and can be easily overestimated during development. Later it can be toned down.
        ice::u32 predicted_resource_count = 10'000;

        //! \brief Creates I/O threads awaiting completion of read requests.
        //!     When a I/O request gets completed, it will be scheduled with 'flags_io_complete' flags.
        //!
        //! \note If the value is greater than '0', the 'flag_io_wait' field is unused.
        //! \note If the value is equal to '0', I/O request tasks will be scheduled using the 'flags_io_wait' flags.
        //!     Such tasks are re-scheduled unless the request is finalized.
        ice::u32 io_dedicated_threads = 1;

        //! \brief Flags to be used when scheduling completed I/O requests.
        ice::TaskFlags flags_io_complete;

        //! \brief Flags to be used when scheduling awaiting I/O requests.
        ice::TaskFlags flags_io_wait;
    };

    class ResourceTracker
    {
    public:
        virtual ~ResourceTracker() noexcept = default;

        virtual auto attach_provider(
            ice::UniquePtr<ice::ResourceProvider> provider
        ) noexcept -> ice::ResourceProvider* = 0;

        virtual auto attach_writer(
            ice::UniquePtr<ice::ResourceWriter> provider
        ) noexcept -> ice::ResourceWriter* = 0;

        //! \brief Synchronize resources with changes reported by providers.
        //!
        //! \note Should be called periodically.
        virtual void sync_resources() noexcept = 0;

        virtual auto find_resource(
            ice::URI const& uri,
            ice::ResourceFlags flags = ice::ResourceFlags::None
        ) const noexcept -> ice::ResourceHandle = 0;

        virtual auto find_resource_relative(
            ice::URI const& uri,
            ice::ResourceHandle const& resource_handle
        ) const noexcept -> ice::ResourceHandle = 0;

        virtual auto filter_resource_uris(
            ice::ResourceFilter const& filter,
            ice::Array<ice::URI>& out_uris
        ) const noexcept -> ice::TaskExpected<ice::u32> = 0;


        virtual auto set_resource(
            ice::URI const& uri,
            ice::ResourceHandle const& resource_handle
        ) noexcept -> ice::Task<ice::ResourceResult> = 0;

        virtual auto load_resource(
            ice::ResourceHandle const& resource_handle
        ) noexcept -> ice::Task<ice::ResourceResult> = 0;

        virtual auto release_resource(
            ice::ResourceHandle const& resource_handle
        ) noexcept -> ice::Task<ice::ResourceResult> = 0;

        virtual auto unload_resource(
            ice::ResourceHandle const& resource_handle
        ) noexcept -> ice::Task<ice::ResourceResult> = 0;

    public:
        virtual auto create_resource(
            ice::URI const& uri
        ) noexcept -> ice::TaskExpected<ice::ResourceHandle> = 0;

        virtual auto write_resource(
            ice::URI const& uri,
            ice::Data data,
            ice::usize write_offset = 0_B
        ) noexcept -> ice::TaskExpected<bool> = 0;

        virtual auto write_resource(
            ice::ResourceHandle const& handle,
            ice::Data data,
            ice::usize write_offset = 0_B
        ) noexcept -> ice::TaskExpected<bool> = 0;
    };

    auto resource_uri(ice::ResourceHandle const& handle) noexcept -> ice::URI const&;
    auto resource_origin(ice::ResourceHandle const& handle) noexcept -> ice::String;
    auto resource_path(ice::ResourceHandle const& handle) noexcept -> ice::String;
    auto resource_meta(ice::ResourceHandle const& handle, ice::Data& out_metadata) noexcept -> ice::Task<ice::Result>;
    auto get_loose_resource(ice::ResourceHandle const& handle) noexcept -> ice::LooseResource const*;

    auto create_resource_tracker(
        ice::Allocator& alloc,
        ice::ResourceTrackerCreateInfo const& create_info
    ) noexcept -> ice::UniquePtr<ice::ResourceTracker>;

    auto resolve_dynlib_path(
        ice::ResourceTracker const& tracker,
        ice::Allocator& alloc,
        ice::String name
    ) noexcept -> ice::HeapString<>;

} // namespace ice
