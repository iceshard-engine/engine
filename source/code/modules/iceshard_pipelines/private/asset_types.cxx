/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/asset_module.hxx>
#include <ice/log_module.hxx>
#include <ice/module_register.hxx>

//#include "mesh_pipeline/mesh_pipeline.hxx"
#include "asset_font.hxx"
#include "asset_image.hxx"
#include "asset_shader.hxx"
#include "pipeline_ui/ip_ui_asset.hxx"

namespace ice
{

    void asset_type_definitions(ice::AssetTypeArchive& asset_type_archive) noexcept
    {
        asset_type_shader_definition(asset_type_archive);
        asset_type_image_definition(asset_type_archive);
        asset_type_font_definition(asset_type_archive);
        asset_type_ui_definition(asset_type_archive);
    }

    struct IceShardPipelinesModule : ice::Module<IceShardPipelinesModule>
    {
        static void v1_archive_api(ice::detail::asset_system::v1::AssetTypeArchiveAPI& api) noexcept
        {
            api.register_types_fn = asset_type_definitions;
        }

        static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiator const& negotiator) noexcept
        {
            ice::LogModule::init(alloc, negotiator);
            return negotiator.register_api(v1_archive_api);
        }
    };

} // namespace ice
