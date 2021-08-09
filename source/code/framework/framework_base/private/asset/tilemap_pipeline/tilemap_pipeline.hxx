#pragma once
#include <ice/asset_pipeline.hxx>
#include <ice/log_tag.hxx>

#include "tilemap_loader.hxx"
#include "tilemap_tmx_oven.hxx"

namespace ice
{

    class IceTiledAssetOven;
    class IceTiledAssetLoader;

    class IceTiledAssetPipeline : public ice::AssetPipeline
    {
    public:
        ~IceTiledAssetPipeline() noexcept override = default;

        auto supported_types() const noexcept -> ice::Span<AssetType const> override;

        bool supports_baking(ice::AssetType type) const noexcept override;

        bool resolve(
            ice::String resource_extension,
            ice::Metadata const& resource_metadata,
            ice::AssetType& out_type,
            ice::AssetStatus& out_status
        ) const noexcept override;

        auto request_oven(
            ice::AssetType type,
            ice::String extension,
            ice::Metadata const& metadata
        ) noexcept -> ice::AssetOven const* override;

        auto request_loader(
            ice::AssetType type
        ) noexcept -> ice::AssetLoader const* override;

    private:
        ice::IceTiledTmxAssetOven _tmx_oven;
        ice::IceTiledAssetLoader _loader;
    };

    constexpr LogTagDefinition LogTag_TiledOven = ice::create_log_tag(ice::LogTag::Asset, "Tiled");

} // namespace ice