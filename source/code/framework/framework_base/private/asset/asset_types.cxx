/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "tilemap/asset_tilemap.hxx"

#include <ice/framework_module.hxx>
#include <ice/game_tilemap.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/asset_module.hxx>
#include <ice/asset_type_archive.hxx>
#include <ice/module_register.hxx>

namespace ice
{

    void iceshard_register_tmx_tilemap_asset_type(
        ice::AssetTypeArchive& type_archive
    ) noexcept
    {
        static ice::String extensions[]{ ".tmx" };

        static ice::AssetTypeDefinition type_definition{
            .resource_extensions = extensions,
            .fn_asset_oven = ice::build::is_windows ? asset_tilemap_oven_tmx : nullptr,
            .fn_asset_loader = asset_tilemap_loader
        };

        type_archive.register_type(ice::AssetType_TileMap, type_definition);
    }

    void iceshard_base_framework_register_asset_types(
        ice::AssetTypeArchive& type_archive
    ) noexcept
    {
        iceshard_register_tmx_tilemap_asset_type(type_archive);
    }

    void iceshard_base_framework_pipeline_api(ice::detail::asset_system::v1::AssetTypeArchiveAPI& api) noexcept
    {
        api.register_types_fn = iceshard_base_framework_register_asset_types;
    }

    struct FrameworkAssetsModuke
    {
        static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiator const& negotiator) noexcept
        {
            return negotiator.register_api(iceshard_base_framework_pipeline_api);
        }
    };

} // namespace ice
