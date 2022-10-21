#include "tilemap/asset_tilemap.hxx"

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
        using ice::detail::asset_system::v1::Constant_APIName_AssetTypeArchive;

        static ice::String extensions[]{ ".tmx" };

        static ice::AssetTypeDefinition type_definition{
            .resource_extensions = extensions,
            .fn_asset_oven = asset_tilemap_oven_tmx,
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

    bool iceshard_base_framework_pipeline_api(
        ice::StringID_Hash name,
        ice::u32 version,
        void** api_ptr
    ) noexcept
    {
        static ice::detail::asset_system::v1::AssetTypeArchiveAPI framework_api{
            .register_types_fn = iceshard_base_framework_register_asset_types
        };

        if (name == ice::stringid_hash(ice::Constant_APIName_AssetTypeArchive) && version == 1)
        {
            *api_ptr = &framework_api;
            return true;
        }
        return false;
    }

    void load_framework_asset_module(
        ice::Allocator* alloc,
        ice::ModuleNegotiatorContext* ctx,
        ice::ModuleNegotiator* negotiator
    ) noexcept
    {
        //ice::register_log_tag(LogTag_TiledOven);

        negotiator->fn_register_module(ctx, ice::stringid_hash(Constant_APIName_AssetTypeArchive), ice::iceshard_base_framework_pipeline_api);
    }

    void unload_framework_asset_module(
        ice::Allocator* alloc
    ) noexcept
    {
    }

    void register_asset_modules(
        ice::Allocator& alloc,
        ice::ModuleRegister& registry
    ) noexcept
    {
        registry.load_module(alloc, load_framework_asset_module, unload_framework_asset_module);
    }

} // namespace ice
