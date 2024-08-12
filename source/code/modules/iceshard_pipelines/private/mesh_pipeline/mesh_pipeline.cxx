/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

//#include "mesh_pipeline.hxx"
//
//namespace ice
//{
//
//    auto IceshardMeshPipeline::supported_types() const noexcept -> ice::Span<AssetCategory const>
//    {
//        static ice::AssetCategory supported_types[]{
//            AssetCategory::Mesh
//        };
//        return supported_types;
//    }
//
//    bool IceshardMeshPipeline::supports_baking(
//        ice::AssetCategory type
//    ) const noexcept
//    {
//        return type == AssetCategory::Mesh;
//    }
//
//    bool IceshardMeshPipeline::resolve(
//        ice::String resource_extension,
//        ice::Metadata const& resource_metadata,
//        ice::AssetCategory& out_type,
//        ice::AssetStatus& out_status
//    ) const noexcept
//    {
//        if (resource_extension == ".fbx"
//            || resource_extension == ".x3d"
//            || resource_extension == ".obj"
//            || resource_extension == ".dae")
//        {
//            out_type = AssetCategory::Mesh;
//            out_status = AssetStatus::Available_Raw;
//            return true;
//        }
//        return false;
//    }
//
//    auto IceshardMeshPipeline::request_oven(
//        ice::AssetCategory type,
//        ice::String extension,
//        ice::Metadata const& metadata
//    ) noexcept -> ice::AssetOven const*
//    {
//        return &_mesh_oven;
//    }
//
//    auto IceshardMeshPipeline::request_loader(
//        ice::AssetCategory type
//    ) noexcept -> ice::AssetLoader const*
//    {
//        return &_mesh_loader;
//    }
//
//} // namespace ice
