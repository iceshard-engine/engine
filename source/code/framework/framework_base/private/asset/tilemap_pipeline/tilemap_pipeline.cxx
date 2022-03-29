//#include "tilemap_pipeline.hxx"
//#include "tilemap_tmx_oven.hxx"
//#include "tilemap_loader.hxx"
//
//#include <ice/assert.hxx>
//
//namespace ice
//{
//
//    auto IceTiledAssetPipeline::supported_types() const noexcept -> ice::Span<AssetType const>
//    {
//        static ice::AssetType asset_types[]{
//            AssetType::TileMap
//        };
//        return asset_types;
//    }
//
//    bool IceTiledAssetPipeline::supports_baking(ice::AssetType type) const noexcept
//    {
//        return true;
//    }
//
//    bool IceTiledAssetPipeline::resolve(
//        ice::String resource_extension,
//        ice::Metadata const& resource_metadata,
//        ice::AssetType& out_type,
//        ice::AssetStatus& out_status
//    ) const noexcept
//    {
//        if (resource_extension == ".tmx")
//        {
//            out_type = AssetType::TileMap;
//            out_status = AssetStatus::Available_Raw;
//            return true;
//        }
//        return false;
//    }
//
//    auto IceTiledAssetPipeline::request_oven(
//        ice::AssetType type,
//        ice::String extension,
//        ice::Metadata const& metadata
//    ) noexcept -> ice::AssetOven const*
//    {
//        return &_tmx_oven;
//    }
//
//    auto IceTiledAssetPipeline::request_loader(
//        ice::AssetType type
//    ) noexcept -> ice::AssetLoader const*
//    {
//        return &_loader;
//    }
//
//} // namespace ice
