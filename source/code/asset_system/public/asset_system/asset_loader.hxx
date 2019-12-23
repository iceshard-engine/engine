#pragma once
#include <asset_system/asset.hxx>
#include <resource/resource_system.hxx>

namespace asset
{

    class AssetLoader
    {
    public:
        virtual ~AssetLoader() noexcept = default;

        //virtual auto create_asset_reference(core::StringView<> basename, resource::URI location) noexcept = 0;

        virtual auto request_asset(asset::Asset asset_reference) noexcept -> asset::AssetStatus = 0;

        virtual auto load_asset(asset::Asset asset_reference, asset::AssetData& asset_data) noexcept -> asset::AssetStatus = 0;

        virtual void release_asset(asset::Asset asset_reference) noexcept = 0;
    };

} // namespace asset
