#pragma once
#include <ice/uri.hxx>
#include <ice/asset.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/span.hxx>

namespace ice
{

    class Resource_v2;
    class ResourceTracker_v2;
    struct ResourceHandle;

    class AssetPipeline;

    class AssetSystem
    {
    public:
        virtual ~AssetSystem() noexcept = default;

        virtual bool add_pipeline(
            ice::StringID_Arg name,
            ice::UniquePtr<AssetPipeline> pipeline
        ) noexcept = 0;

        virtual bool remove_pipeline(
            ice::StringID_Arg name
        ) noexcept = 0;

        virtual void bind_resources(
            ice::Span<ice::ResourceHandle*> resources
        ) noexcept = 0;

        virtual bool bind_resource(
            ice::AssetType type,
            ice::StringID_Arg name,
            ice::ResourceHandle* resource
        ) noexcept = 0;

        virtual auto request(
            ice::AssetType type,
            ice::StringID_Arg name
        ) noexcept -> Asset = 0;

        virtual auto load(
            ice::AssetType type,
            ice::ResourceHandle* resource
        ) noexcept -> Asset = 0;

        virtual void release(
            ice::Asset asset
        ) noexcept = 0;
    };

    auto create_asset_system(
        ice::Allocator& alloc,
        ice::ResourceTracker_v2& resource_system
    ) noexcept -> ice::UniquePtr<ice::AssetSystem>;

} // namespace ice
