#pragma once
#include <ice/shard.hxx>
#include <ice/mem_data.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/container_types.hxx>
#include <ice/resource_types.hxx>
#include <ice/resource_flags.hxx>
#include <ice/resource_status.hxx>
#include <ice/task.hxx>

namespace ice
{

    struct ResourceResult
    {
        ice::ResourceStatus resource_status;
        ice::Resource_v2 const* resource;
        ice::Data data;
    };

    struct ResourceTrackerCreateInfo
    {
        bool create_loader_thread = false;
        ice::ResourceFlagsCompareFn* compare_fn = ice::default_resource_flags_compare_function;
    };

    class ResourceTracker
    {
    public:
        virtual ~ResourceTracker() noexcept = default;

        virtual bool attach_provider(ice::ResourceProvider* provider) noexcept = 0;
        virtual bool detach_provider(ice::ResourceProvider* provider) noexcept = 0;

        virtual void refresh_providers() noexcept = 0;


        virtual auto find_resource(
            ice::URI const& uri,
            ice::ResourceFlags flags = ice::ResourceFlags::None
        ) const noexcept -> ice::ResourceHandle* = 0;

        virtual auto find_resource_relative(
            ice::URI const& uri,
            ice::ResourceHandle* resource_handle
        ) const noexcept -> ice::ResourceHandle* = 0;


        virtual auto set_resource(
            ice::URI const& uri,
            ice::ResourceHandle* resource_handle
        ) noexcept -> ice::Task<ice::ResourceResult> = 0;

        virtual auto load_resource(
            ice::ResourceHandle* resource_handle
        ) noexcept -> ice::Task<ice::ResourceResult> = 0;

        virtual auto release_resource(
            ice::ResourceHandle* resource_handle
        ) noexcept -> ice::Task<ice::ResourceResult> = 0;

        virtual auto unload_resource(
            ice::ResourceHandle* resource_handle
        ) noexcept -> ice::Task<ice::ResourceResult> = 0;
    };

    auto resource_origin(ice::ResourceHandle const* handle) noexcept -> ice::String;

    auto resource_path(ice::ResourceHandle const* handle) noexcept -> ice::String;

    auto create_resource_tracker(
        ice::Allocator& alloc,
        ice::ResourceTrackerCreateInfo const& create_info
    ) noexcept -> ice::UniquePtr<ice::ResourceTracker>;

} // namespace ice
