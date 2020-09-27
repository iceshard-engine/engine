#pragma once
#include <asset_system/asset_loader.hxx>
#include <asset_system/assets/asset_texture.hxx>

namespace iceshard
{

    class StbTextureLoader final : public asset::AssetLoader
    {
    public:
        StbTextureLoader(core::allocator& alloc) noexcept;
        ~StbTextureLoader() noexcept;

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
        core::allocator& _mesh_allocator;

        core::pod::Hash<asset::AssetStatus> _texture_status;
        core::pod::Hash<void*> _texture_pixels;
        //core::pod::Hash<iceshard::renderer::v1::Model> _models;
    };

} // namespace iceshard
