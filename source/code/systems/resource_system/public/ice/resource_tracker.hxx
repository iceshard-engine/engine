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
    struct ResourceHandle;

    auto resource_origin(ice::ResourceHandle const* handle) noexcept -> ice::Utf8String;
    auto resource_object_DEPRECATED(ice::ResourceHandle const* handle) noexcept -> ice::Resource_v2 const*;

    enum class ResourceFlags_v2 : ice::u32
    {
        None,
        PC,
        Console,
        Handheld,
        Phone
    };


    class ResourceTracker_v2
    {
    public:
        virtual ~ResourceTracker_v2() noexcept = default;

        virtual bool attach_provider(ice::ResourceProvider_v2* provider) noexcept = 0;
        virtual bool detach_provider(ice::ResourceProvider_v2* provider) noexcept = 0;

        virtual void refresh_providers() noexcept = 0;


        virtual void gather_resources_DEPRECATED(
            ice::pod::Array<ice::ResourceHandle*>& handles
        ) const noexcept = 0;

        virtual auto find_resource(
            ice::URI_v2 const& uri,
            ice::ResourceFlags_v2 flags = ice::ResourceFlags_v2::None
        ) const noexcept -> ice::ResourceHandle* = 0;


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
    };

    auto create_resource_tracker(
        ice::Allocator& alloc,
        bool async = false
    ) noexcept -> ice::UniquePtr<ice::ResourceTracker_v2>;

} // namespace ice
