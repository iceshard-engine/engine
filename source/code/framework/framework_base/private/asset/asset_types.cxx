/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "tilemap/asset_tilemap.hxx"

#include <ice/framework_module.hxx>
#include <ice/game_tilemap.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/asset_module.hxx>
#include <ice/asset_category_archive.hxx>
#include <ice/module_register.hxx>

namespace ice
{

    void iceshard_register_tmx_tilemap_asset_category(
        ice::AssetCategoryArchive& type_archive
    ) noexcept
    {
        static ice::String extensions[]{ ".tmx" };

        static ice::AssetCategoryDefinition definition{
            .resource_extensions = extensions,
            // .fn_asset_oven = ice::build::is_windows ? asset_tilemap_oven_tmx : nullptr,
            .fn_asset_loader = asset_tilemap_loader
        };

        type_archive.register_category(ice::AssetCategory_TileMap, definition);
    }

    void iceshard_base_framework_register_asset_categories(
        ice::AssetCategoryArchive& type_archive,
        ice::ModuleQuery const& module_query
    ) noexcept
    {
        iceshard_register_tmx_tilemap_asset_category(type_archive);
    }

    void iceshard_base_framework_pipeline_api(ice::detail::asset_system::v1::AssetArchiveAPI& api) noexcept
    {
        api.fn_register_categories = iceshard_base_framework_register_asset_categories;
    }

    struct FrameworkAssetsModule
    {
        static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiator auto const& negotiator) noexcept
        {
            return negotiator.register_api(iceshard_base_framework_pipeline_api);
        }
    };

} // namespace ice
