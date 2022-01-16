#pragma once
#include <ice/uri.hxx>
#include <ice/shard.hxx>
#include <ice/data.hxx>
#include <ice/pod/array.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/resource_meta.hxx>
#include <ice/resource_action.hxx>

namespace ice
{

    class Resource_v2;
    class ResourceProvider_v2;

    using Res = ResourceHandle*;

    enum class ResourceFlags_v2 : ice::u32
    {
        None,
        PC,
        Console,
        Handheld,
        Phone
    };

    struct ResourceHandle
    {
        ice::ResourceProvider_v2* provider;
        ice::Resource_v2 const* resource;
        ice::Memory data;

        ice::ResourceStatus_v2 status;
        ice::u32 refcount;
        ice::u32 usecount;
    };

    inline auto resource_object(ice::ResourceHandle const* handle) noexcept -> ice::Resource_v2 const*
    {
        return handle->resource;
    }

    auto resource_origin(ice::ResourceHandle const* handle) noexcept -> ice::Utf8String;

    auto resource_object(ice::ResourceHandle const* handle) noexcept -> ice::Resource_v2 const*;

    class ResourceTracker_v2
    {
    public:
        virtual ~ResourceTracker_v2() noexcept = default;

        virtual bool attach_provider(ice::ResourceProvider_v2* provider) noexcept = 0;
        virtual bool detach_provider(ice::ResourceProvider_v2* provider) noexcept = 0;

        virtual void refresh_providers() noexcept = 0;

        virtual void gather_resources(
            ice::pod::Array<ice::ResourceHandle*>& handles
        ) const noexcept = 0;

        virtual auto find_resource(
            ice::URI_v2 const& uri,
            ice::ResourceFlags_v2 flags = ice::ResourceFlags_v2::None
        ) const noexcept -> ice::ResourceHandle* = 0;


        //virtual auto create_resource(
        //    ice::URI_v2 const& uri,
        //    ice::Metadata const& metadata,
        //    ice::Data data
        //) noexcept -> ice::Task<ice::ResourceActionResult> = 0;

        virtual auto set_resource(
            ice::URI_v2 const& uri,
            ice::ResourceHandle* resource_handle
        ) noexcept -> ice::Task<ice::ResourceActionResult> = 0;


        virtual auto load_resource(
            ice::ResourceHandle* resource_handle
        ) noexcept -> ice::Task<ice::ResourceActionResult> = 0;

        virtual auto release_resource(
            ice::ResourceHandle* resource_handle
        ) noexcept -> ice::Task<ice::ResourceActionResult> = 0;

        virtual auto unload_resource(
            ice::ResourceHandle* resource_handle
        ) noexcept -> ice::Task<ice::ResourceActionResult> = 0;

        //virtual auto update_resource(
        //    ice::Resource_v2 const* resource_handle,
        //    ice::Metadata const* metadata,
        //    ice::Data data
        //) noexcept -> ice::Task<ice::ResourceActionResult> = 0;

    public:
        // Probably prepare an interface or concept for this method?
        //virtual void query_shards(ice::pod::Array<ice::Shard>& out_shards) noexcept = 0;
    };

    auto create_resource_tracker(
        ice::Allocator& alloc,
        bool async = false
    ) noexcept -> ice::UniquePtr<ice::ResourceTracker_v2>;

} // namespace ice
