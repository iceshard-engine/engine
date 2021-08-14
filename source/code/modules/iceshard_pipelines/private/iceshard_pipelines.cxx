#include <ice/asset_module.hxx>
#include <ice/allocator.hxx>
#include <ice/module_register.hxx>
#include <ice/log_module.hxx>

#include "mesh_pipeline/mesh_pipeline.hxx"
#include "image_pipeline/image_pipeline.hxx"

namespace ice
{

    class IceshardShaderPipeline final : public ice::AssetPipeline, public ice::AssetLoader
    {
    public:
        auto supported_types() const noexcept -> ice::Span<AssetType const> override
        {
            static AssetType types[]{
                AssetType::Shader
            };
            return types;
        }

        bool supports_baking(ice::AssetType type) const noexcept override
        {
            return false;
        }

        bool resolve(
            ice::String resource_extension,
            ice::Metadata const& resource_metadata,
            ice::AssetType& out_type,
            ice::AssetStatus& out_status
        ) const noexcept override
        {
            if (resource_extension == ".spv")
            {
                out_type = AssetType::Shader;
                out_status = AssetStatus::Available;
                return true;
            }
            return false;
        }

        auto request_oven(
            ice::AssetType type,
            ice::String extension,
            ice::Metadata const& metadata
        ) noexcept -> ice::AssetOven const* override
        {
            return nullptr;
        }

        auto request_loader(
            ice::AssetType type
        ) noexcept -> ice::AssetLoader const* override
        {
            return this;
        }

        auto load(
            ice::AssetType type,
            ice::Data data,
            ice::Allocator& alloc,
            ice::Memory& out_data
        ) const noexcept -> ice::AssetStatus override
        {
            out_data.size = sizeof(Data);
            out_data.alignment = alignof(Data);
            out_data.location = alloc.allocate(out_data.size, out_data.alignment);

            Data* shader_data = reinterpret_cast<Data*>(out_data.location);
            shader_data->location = data.location;
            shader_data->size = data.size;
            shader_data->alignment = data.alignment;
            return ice::AssetStatus::Loaded;
        }
    };

    auto create_shader_pipeline(ice::Allocator& alloc) noexcept -> ice::AssetPipeline*
    {
        return alloc.make<IceshardShaderPipeline>();
    }

    void destroy_shader_pipeline(ice::Allocator& alloc, ice::AssetPipeline* pipeline) noexcept
    {
        alloc.destroy(pipeline);
    }

    auto get_shader_pipeline_name() noexcept -> ice::StringID
    {
        return "ice.shader.Placeholder"_sid;
    };

    bool iceshard_shader_pipeline_api(
        ice::StringID_Hash name,
        ice::u32 version,
        void** api_ptr
    ) noexcept
    {

        static ice::detail::asset_system::v1::AssetModuleAPI shader_api{
            .name_fn = get_shader_pipeline_name,
            .create_pipeline_fn = create_shader_pipeline,
            .destroy_pipeline_fn = destroy_shader_pipeline,
        };

        if (name == "ice.asset_module"_sid_hash && version == 1)
        {
            *api_ptr = &shader_api;
            return true;
        }
        return false;
    }

    auto create_image_pipeline(ice::Allocator& alloc) noexcept -> ice::AssetPipeline*
    {
        return alloc.make<IceshardImagePipeline>();
    }

    auto destroy_image_pipeline(ice::Allocator& alloc, ice::AssetPipeline* pipeline) noexcept
    {
        alloc.destroy(pipeline);
    }

    auto get_image_pipeline_name() noexcept -> ice::StringID
    {
        return "ice.image.StbLoader"_sid;
    };

    bool iceshard_image_pipeline_api(
        ice::StringID_Hash name,
        ice::u32 version,
        void** api_ptr
    ) noexcept
    {
        static ice::detail::asset_system::v1::AssetModuleAPI image_api{
            .name_fn = get_image_pipeline_name,
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

    auto get_mesh_pipeline_name() noexcept -> ice::StringID
    {
        return "ice.mesh.Assimp"_sid;
    };

    bool iceshard_mesh_pipeline_api(
        ice::StringID_Hash name,
        ice::u32 version,
        void** api_ptr
    ) noexcept
    {
        static ice::detail::asset_system::v1::AssetModuleAPI mesh_api{
            .name_fn = get_mesh_pipeline_name,
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
        negotiator->fn_register_module(ctx, "ice.asset_module"_sid_hash, ice::iceshard_image_pipeline_api);
        negotiator->fn_register_module(ctx, "ice.asset_module"_sid_hash, ice::iceshard_shader_pipeline_api);
        return true;
    }

} // namespace ice

extern "C"
{

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

} // extern "C"
