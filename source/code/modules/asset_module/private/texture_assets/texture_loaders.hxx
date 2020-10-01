#pragma once
#include <asset_system/asset_loader.hxx>
#include <asset_system/assets/asset_texture.hxx>
#include <iceshard/renderer/render_model.hxx>

namespace iceshard
{

    class StbTextureLoader final : public asset::AssetLoader
    {
    public:
        StbTextureLoader(core::allocator& alloc) noexcept;
        ~StbTextureLoader() noexcept;

        bool supported_raw_assets() const noexcept override { return false; }

        auto supported_asset_types() const noexcept -> core::pod::Array<asset::AssetType> const& override;

        auto request_asset(asset::Asset asset) noexcept -> asset::AssetStatus override;

        auto load_asset(
            asset::Asset asset,
            resource::ResourceMetaView meta,
            core::data_view resource_data,
            asset::AssetData& result_data
        ) noexcept -> asset::AssetStatus override;

        bool release_asset(asset::Asset asset) noexcept override;

    private:
        core::allocator& _allocator;

        core::pod::Hash<asset::AssetStatus> _texture_status;
        core::pod::Hash<iceshard::renderer::data::Texture> _textures;
    };

} // namespace iceshard
