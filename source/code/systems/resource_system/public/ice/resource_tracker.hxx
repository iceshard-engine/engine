#pragma once
#include <ice/shard.hxx>
#include <ice/data.hxx>
#include <ice/task.hxx>
#include <ice/pod/array.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/resource_types.hxx>
#include <ice/resource_flags.hxx>
#include <ice/resource_status.hxx>

namespace ice
{

    struct ResourceResult
    {
        ice::ResourceStatus resource_status;
        ice::Resource_v2 const* resource;
        ice::Data data;
    };

    class ResourceTracker
    {
    public:
        virtual ~ResourceTracker() noexcept = default;

        virtual bool attach_provider(ice::ResourceProvider* provider) noexcept = 0;
        virtual bool detach_provider(ice::ResourceProvider* provider) noexcept = 0;

        virtual void refresh_providers() noexcept = 0;


        virtual void gather_resources_DEPRECATED(
            ice::pod::Array<ice::ResourceHandle*>& handles
        ) const noexcept = 0;

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

    auto resource_origin(ice::ResourceHandle const* handle) noexcept -> ice::Utf8String;
    auto resource_object_DEPRECATED(ice::ResourceHandle const* handle) noexcept -> ice::Resource_v2 const*;

    auto create_resource_tracker(
        ice::Allocator& alloc,
        bool async = false
    ) noexcept -> ice::UniquePtr<ice::ResourceTracker>;

} // namespace ice
