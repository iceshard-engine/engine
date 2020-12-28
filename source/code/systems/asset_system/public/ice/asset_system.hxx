#pragma once
#include <ice/uri.hxx>
#include <ice/asset.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/span.hxx>

namespace ice
{

    class Resource;

    class ResourceSystem;

    class AssetPipeline;

    class AssetSystem
    {
    public:
        virtual ~AssetSystem() noexcept = default;

        virtual bool add_pipeline(
            ice::StringID name,
            ice::UniquePtr<AssetPipeline> pipeline
        ) noexcept = 0;

        virtual bool remove_pipeline(
            ice::StringID name
        ) noexcept = 0;

        virtual void bind_resources(
            ice::Span<ice::Resource*> resources
        ) noexcept = 0;

        virtual bool bind_resource(
            ice::AssetType type,
            ice::StringID name,
            ice::Resource* resource
        ) noexcept = 0;

        virtual auto request(
            ice::AssetType type,
            ice::StringID name
        ) noexcept -> Asset = 0;

        virtual auto load(
            ice::AssetType type,
            ice::Resource* resource
        ) noexcept -> Asset = 0;

        virtual void release(
            ice::Asset asset
        ) noexcept = 0;
    };

    auto default_asset_system(
        ice::Allocator& alloc,
        ice::ResourceSystem& resource_system
    ) noexcept -> ice::UniquePtr<ice::AssetSystem>;

} // namespace ice
