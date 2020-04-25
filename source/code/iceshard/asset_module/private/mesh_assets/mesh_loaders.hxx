#pragma once
#include <asset_system/asset_loader.hxx>
#include <asset_system/assets/asset_mesh.hxx>

namespace iceshard
{

    class AssimpMeshLoader final : public asset::AssetLoader
    {
    public:
        AssimpMeshLoader(core::allocator& alloc) noexcept;
        ~AssimpMeshLoader() noexcept;

        auto supported_asset_types() const noexcept -> core::pod::Array<asset::AssetType> const& override;

        auto request_asset(asset::Asset asset) noexcept -> asset::AssetStatus override;

        auto load_asset(
            asset::Asset asset,
            resource::ResourceMetaView meta,
            core::data_view resource_data,
            asset::AssetData & result_data
        ) noexcept -> asset::AssetStatus override;

        void release_asset(asset::Asset asset) noexcept override;

    private:
        core::allocator& _allocator;
        core::allocator& _mesh_allocator;

        core::pod::Array<asset::v1::Mesh> _meshes;
    };

} // namespace iceshard
