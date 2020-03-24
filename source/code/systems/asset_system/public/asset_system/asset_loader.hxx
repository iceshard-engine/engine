#pragma once
#include <asset_system/asset.hxx>
#include <resource/resource_meta.hxx>

namespace asset
{

    class AssetLoader
    {
    public:
        virtual ~AssetLoader() noexcept = default;

        virtual auto supported_asset_types() const noexcept -> core::pod::Array<asset::AssetType> const& = 0;

        virtual auto request_asset(asset::Asset asset) noexcept -> asset::AssetStatus = 0;

        virtual auto load_asset(
            asset::Asset asset,
            resource::ResourceMetaView meta,
            core::data_view resource_data,
            asset::AssetData& result_data
        ) noexcept -> asset::AssetStatus = 0;

        virtual void release_asset(asset::Asset asset) noexcept = 0;
    };

} // namespace asset
