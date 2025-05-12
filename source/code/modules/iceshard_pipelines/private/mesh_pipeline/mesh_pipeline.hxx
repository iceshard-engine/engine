/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
//#include <ice/asset_pipeline.hxx>
#include "mesh_oven.hxx"
#include "mesh_loader.hxx"

namespace ice
{

    //class IceshardMeshPipeline final : public ice::AssetPipeline
    //{
    //public:
    //    auto supported_types() const noexcept -> ice::Span<AssetCategory const> override;

    //    bool supports_baking(
    //        ice::AssetCategory type
    //    ) const noexcept override;

    //    bool resolve(
    //        ice::String resource_extension,
    //        ice::Metadata const& resource_metadata,
    //        ice::AssetCategory& out_type,
    //        ice::AssetStatus& out_status
    //    ) const noexcept override;

    //    auto request_oven(
    //        ice::AssetCategory type,
    //        ice::String extension,
    //        ice::Metadata const& metadata
    //    ) noexcept -> ice::AssetOven const* override;

    //    auto request_loader(
    //        ice::AssetCategory type
    //    ) noexcept -> ice::AssetLoader const* override;

    //private:
    //    ice::IceshardMeshOven _mesh_oven;
    //    ice::IceshardMeshLoader _mesh_loader;
    //};

} // namespace ice
