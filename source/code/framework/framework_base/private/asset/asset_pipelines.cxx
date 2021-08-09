#pragma once
#include "asset_pipelines.hxx"
#include "tilemap_pipeline/tilemap_pipeline.hxx"
#include "tilemap_pipeline/tilemap_tmx_oven.hxx"

#include <ice/unique_ptr.hxx>
#include <ice/asset_module.hxx>
#include <ice/module_register.hxx>

namespace ice
{

    auto get_tiled_pipeline_name() noexcept -> ice::StringID
    {
        return "ice.tilemap.Tiled"_sid;
    };

    auto create_tiled_pipeline(ice::Allocator& alloc) noexcept -> ice::AssetPipeline*
    {
        return alloc.make<IceTiledAssetPipeline>();
    }

    auto destroy_tiled_pipeline(ice::Allocator& alloc, ice::AssetPipeline* pipeline) noexcept
    {
        alloc.destroy(pipeline);
    }

    bool iceshard_tiled_pipeline_api(
        ice::StringID_Hash name,
        ice::u32 version,
        void** api_ptr
    ) noexcept
    {
        static ice::detail::asset_system::v1::AssetModuleAPI mesh_api{
            .name_fn = get_tiled_pipeline_name,
            .create_pipeline_fn = create_tiled_pipeline,
            .destroy_pipeline_fn = destroy_tiled_pipeline,
        };

        if (name == "ice.asset_module"_sid_hash && version == 1)
        {
            *api_ptr = &mesh_api;
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
        ice::register_log_tag(LogTag_TiledOven);

        negotiator->fn_register_module(ctx, "ice.asset_module"_sid_hash, ice::iceshard_tiled_pipeline_api);
    }

    void unload_frameworm_asset_module(
        ice::Allocator* alloc
    ) noexcept
    {
    }

    void register_asset_modules(
        ice::Allocator& alloc,
        ice::ModuleRegister& registry
    ) noexcept
    {
        registry.load_module(alloc, load_framework_asset_module, unload_frameworm_asset_module);
    }

} // namespace ice
