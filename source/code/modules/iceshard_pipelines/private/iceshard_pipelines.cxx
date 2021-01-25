#include <ice/asset_module.hxx>
#include <ice/allocator.hxx>
#include <ice/module_register.hxx>
#include <ice/log_module.hxx>

#include "mesh_pipeline/mesh_pipeline.hxx"
#include "image_pipeline/image_pipeline.hxx"

namespace ice
{

    auto create_image_pipeline(ice::Allocator& alloc) noexcept -> ice::AssetPipeline*
    {
        return alloc.make<IceshardImagePipeline>();
    }

    auto destroy_image_pipeline(ice::Allocator& alloc, ice::AssetPipeline* pipeline) noexcept
    {
        alloc.destroy(pipeline);
    }

    bool iceshard_image_pipeline_api(
        ice::StringID_Hash name,
        ice::u32 version,
        void** api_ptr
    ) noexcept
    {
        static ice::detail::asset_system::v1::AssetModuleAPI image_api{
            .create_pipeline_fn = create_image_pipeline,
            .destroy_pipeline_fn = destroy_image_pipeline,
        };

        if (name == "ice.asset_module"_sid_hash && version == 1)
        {
            *api_ptr = &image_api;
            return true;
        }
        return false;
    }

    auto create_mesh_pipeline(ice::Allocator& alloc) noexcept -> ice::AssetPipeline*
    {
        return alloc.make<IceshardMeshPipeline>();
    }

    auto destroy_mesh_pipeline(ice::Allocator& alloc, ice::AssetPipeline* pipeline) noexcept
    {
        alloc.destroy(pipeline);
    }

    bool iceshard_mesh_pipeline_api(
        ice::StringID_Hash name,
        ice::u32 version,
        void** api_ptr
    ) noexcept
    {
        static ice::detail::asset_system::v1::AssetModuleAPI mesh_api{
            .create_pipeline_fn = create_mesh_pipeline,
            .destroy_pipeline_fn = destroy_mesh_pipeline,
        };

        if (name == "ice.asset_module"_sid_hash && version == 1)
        {
            *api_ptr = &mesh_api;
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

        negotiator->fn_register_module(ctx, "ice.asset_module"_sid_hash, ice::iceshard_mesh_pipeline_api);
        return true;
    }

} // namespace ice

extern "C"
{

    __declspec(dllexport) bool ice_module_load(
        ice::Allocator* alloc,
        ice::ModuleNegotiatorContext* ctx,
        ice::ModuleNegotiator* negotiator
    )
    {
        return ice::ice_module_load(alloc, ctx, negotiator);
    }

    __declspec(dllexport) bool ice_module_unload(
        ice::Allocator* alloc
    )
    {
        return true;
    }

} // extern "C"
