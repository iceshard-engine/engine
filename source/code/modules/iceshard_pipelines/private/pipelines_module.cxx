/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/asset_module.hxx>
#include <ice/log_module.hxx>
#include <ice/module_register.hxx>

//#include "mesh_pipeline/mesh_pipeline.hxx"
#include "asset_font.hxx"
#include "asset_image.hxx"
#include "pipeline_ui/ip_ui_asset.hxx"

namespace ice
{

    struct IceShardPipelinesModule : ice::Module<IceShardPipelinesModule>
    {
        static void v1_archive_api(ice::detail::asset_system::v1::AssetArchiveAPI& api) noexcept;

        static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiator auto const& negotiator) noexcept
        {
            ice::LogModule::init(alloc, negotiator);
            return negotiator.register_api(v1_archive_api);
        }

        IS_WORKAROUND_MODULE_INITIALIZATION(IceShardPipelinesModule);
    };

    void IceShardPipelinesModule::v1_archive_api(ice::detail::asset_system::v1::AssetArchiveAPI& api) noexcept
    {
        api.fn_register_categories = ice::asset_category_image_definition;
    }

} // namespace ice
