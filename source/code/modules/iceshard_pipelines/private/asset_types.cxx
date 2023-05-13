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

    bool asset_type_definitions_api(
        ice::StringID_Hash name,
        ice::u32 version,
        void** api_ptr
    ) noexcept
    {
        using ice::detail::asset_system::v1::AssetTypeArchiveAPI;
        using ice::detail::asset_system::v1::Constant_APIName_AssetTypeArchive;

        static AssetTypeArchiveAPI asset_type_archive_api{
            .register_types_fn = asset_type_definitions
        };

        if (name == ice::stringid_hash(Constant_APIName_AssetTypeArchive) && version == 1)
        {
            *api_ptr = &asset_type_archive_api;
            return true;
        }
        return false;
    }

    bool ice_module_load(
        ice::Allocator* alloc,
        ice::ModuleNegotiatorContext* ctx,
        ice::ModuleNegotiator* negotiator
    ) noexcept
    {
        ice::initialize_log_module(ctx, negotiator);

        using ice::detail::asset_system::v1::Constant_APIName_AssetTypeArchive;

        negotiator->fn_register_module(ctx, ice::stringid_hash(Constant_APIName_AssetTypeArchive), ice::asset_type_definitions_api);
        return true;
    }

} // namespace ice

extern "C"
{
    // #TODO: https://github.com/iceshard-engine/engine/issues/92
#if ISP_WINDOWS
    __declspec(dllexport) void ice_module_load(
        ice::Allocator* alloc,
        ice::ModuleNegotiatorContext* ctx,
        ice::ModuleNegotiator* negotiator
    )
    {
        ice::ice_module_load(alloc, ctx, negotiator);
    }

    __declspec(dllexport) void ice_module_unload(
        ice::Allocator* alloc
    )
    {
    }
#endif // #if ISP_WINDOWS

} // extern "C"
