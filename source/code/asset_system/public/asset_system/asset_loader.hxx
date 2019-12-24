#pragma once
#include <asset_system/asset.hxx>

namespace asset
{

    class AssetLoader
    {
    public:
        virtual ~AssetLoader() noexcept = default;

        virtual auto request_asset(asset::Asset asset_reference) noexcept -> asset::AssetStatus = 0;

        virtual auto load_asset(asset::Asset asset_reference, asset::AssetData& asset_data) noexcept -> asset::AssetStatus = 0;

        virtual void release_asset(asset::Asset asset_reference) noexcept = 0;
    };

} // namespace asset
