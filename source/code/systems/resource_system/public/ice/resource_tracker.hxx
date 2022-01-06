#pragma once
#include <ice/uri.hxx>
#include <ice/shard.hxx>
#include <ice/data.hxx>
#include <ice/pod/array.hxx>
#include <ice/resource_meta.hxx>
#include <ice/resource_action.hxx>

namespace ice
{

    class Resource_v2;
    class ResourceProvider_v2;

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
        virtual void reset_providers() noexcept = 0;


        virtual auto find_resource(
            ice::URI const& uri,
            ice::ResourceFlags_v2 flags = ice::ResourceFlags_v2::None
        ) const noexcept -> ice::Resource_v2 const* = 0;


        virtual auto create_resource(
            ice::URI const& uri,
            ice::Metadata const& metadata,
            ice::Data data
        ) noexcept -> ice::Task<ice::ResourceAction_v2::Result> = 0;

        virtual auto set_resource(
            ice::URI const& uri,
            ice::Resource_v2 const* resource,
            bool replace = false
        ) noexcept -> ice::Task<ice::ResourceAction_v2::Result> = 0;


        virtual auto load_resource(
            ice::Resource_v2 const* resource
        ) noexcept -> ice::Task<ice::ResourceAction_v2::Result> = 0;

        virtual auto release_resource(
            ice::Resource_v2 const* resource
        ) noexcept -> ice::Task<ice::ResourceAction_v2::Result> = 0;

        virtual auto unload_resource(
            ice::Resource_v2 const* resource
        ) noexcept -> ice::Task<ice::ResourceAction_v2::Result> = 0;

        virtual auto update_resource(
            ice::Resource_v2 const* resource,
            ice::Metadata const* metadata,
            ice::Data data
        ) noexcept -> ice::Task<ice::ResourceAction_v2::Result> = 0;

    public:
        // Probably prepare an interface or concept for this method?
        //virtual void query_shards(ice::pod::Array<ice::Shard>& out_shards) noexcept = 0;
    };

} // namespace ice
