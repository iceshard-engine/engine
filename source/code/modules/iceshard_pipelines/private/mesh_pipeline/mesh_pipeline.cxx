#include "mesh_pipeline.hxx"

namespace ice
{

    auto IceshardMeshPipeline::supported_types() const noexcept -> ice::Span<AssetType>
    {
        static ice::AssetType supported_types[]{
            AssetType::Mesh
        };
        return supported_types;
    }

    bool IceshardMeshPipeline::supports_baking(
        ice::AssetType type
    ) const noexcept
    {
        return type == AssetType::Mesh;
    }

    bool IceshardMeshPipeline::resolve(
        ice::String resource_extension,
        ice::Metadata resource_metadata,
        ice::AssetType& out_type,
        ice::AssetStatus& out_status
    ) noexcept
    {
        if (resource_extension == ".fbx"
            || resource_extension == ".x3d"
            || resource_extension == ".obj"
            || resource_extension == ".dae")
        {
            out_type = AssetType::Mesh;
            out_status = AssetStatus::Available_Raw;
            return true;
        }
        return false;
    }

    auto IceshardMeshPipeline::request_oven(
        ice::AssetType type
    ) noexcept -> ice::AssetOven*
    {
        return &_mesh_oven;
    }

    auto IceshardMeshPipeline::request_loader(
        ice::AssetType type
    ) noexcept -> ice::AssetLoader*
    {
        return &_mesh_loader;
    }

} // namespace ice
