#pragma once
#include <ice/task.hxx>
#include <ice/asset.hxx>
#include <ice/asset_type.hxx>
#include <ice/mem_unique_ptr.hxx>

namespace ice
{

    class AssetRequest;
    class AssetTypeArchive;
    class ResourceTracker;

    class AssetStorage
    {
    public:
        virtual ~AssetStorage() = default;

        virtual auto request(
            ice::AssetType type,
            ice::String name,
            ice::AssetState requested_state
        ) noexcept -> ice::Task<ice::Asset> = 0;

        virtual auto aquire_request(
            ice::AssetType type,
            ice::AssetState requested_state
        ) noexcept -> ice::AssetRequest* = 0;

        virtual auto release(
            ice::Asset asset
        ) noexcept -> ice::Task<> = 0;
    };

    auto create_asset_storage(
        ice::Allocator& alloc,
        ice::ResourceTracker& resource_tracker,
        ice::UniquePtr<ice::AssetTypeArchive> asset_archive
    ) noexcept -> ice::UniquePtr<ice::AssetStorage>;

} // namespace ice
