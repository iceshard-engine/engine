#include <ice/asset_module.hxx>
#include <ice/allocator.hxx>
#include <ice/module_register.hxx>
#include <ice/log_module.hxx>

#include <ice/render/render_shader.hxx>
#include <ice/render/render_image.hxx>

#include <ice/asset.hxx>
#include <ice/asset_type_archive.hxx>

#include "mesh_pipeline/mesh_pipeline.hxx"
#include "image_pipeline/image_pipeline.hxx"

namespace ice
{

    auto asset_shader_state(
        void*,
        ice::AssetTypeDefinition const&,
        ice::Metadata const& metadata,
        ice::URI const& uri
    ) noexcept -> ice::AssetState
    {
        return AssetState::Baked;
    }

    bool asset_shader_loader(
        void*,
        ice::Allocator& alloc,
        ice::AssetStorage&,
        ice::Metadata const& meta,
        ice::Data data,
        ice::Memory& out_data
    ) noexcept
    {
        out_data.size = sizeof(Data);
        out_data.alignment = alignof(Data);
        out_data.location = alloc.allocate(out_data.size, out_data.alignment);

        Data* shader_data = reinterpret_cast<Data*>(out_data.location);
        shader_data->location = data.location;
        shader_data->size = data.size;
        shader_data->alignment = data.alignment;
        return true;
    }

    void asset_type_shader_definition(ice::AssetTypeArchive& asset_type_archive) noexcept
    {
        using ice::detail::asset_system::v1::Constant_APIName_AssetTypeArchive;

        static ice::Utf8String extensions[]{ u8".spv" };

        static ice::AssetTypeDefinition type_definition{
            .resource_extensions = extensions,
            .fn_asset_state = asset_shader_state,
            .fn_asset_loader = asset_shader_loader
        };

        asset_type_archive.register_type(ice::render::AssetType_Shader, type_definition);
    }

    void asset_type_image_definition(ice::AssetTypeArchive& asset_type_archive) noexcept
    {
        using ice::detail::asset_system::v1::Constant_APIName_AssetTypeArchive;

        static ice::Utf8String extensions[]{ u8".jpg", u8".png", u8".jpeg", u8".bmp" };

        static ice::AssetTypeDefinition type_definition{
            .resource_extensions = extensions,
            .fn_asset_oven = asset_image_oven,
            .fn_asset_loader = asset_image_loader
        };

        asset_type_archive.register_type(ice::render::AssetType_Texture2D, type_definition);
    }

    void asset_type_definitions(ice::AssetTypeArchive& asset_type_archive) noexcept
    {
        asset_type_shader_definition(asset_type_archive);
        asset_type_image_definition(asset_type_archive);
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
