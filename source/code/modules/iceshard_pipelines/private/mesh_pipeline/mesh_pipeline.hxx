#pragma once
#include <ice/asset_pipeline.hxx>
#include "mesh_oven.hxx"
#include "mesh_loader.hxx"

namespace ice
{

    class IceshardMeshPipeline final : public ice::AssetPipeline
    {
    public:
        auto supported_types() const noexcept -> ice::Span<AssetType const> override;

        bool supports_baking(
            ice::AssetType type
        ) const noexcept override;

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
        ice::IceshardMeshOven _mesh_oven;
        ice::IceshardMeshLoader _mesh_loader;
    };

} // namespace ice
