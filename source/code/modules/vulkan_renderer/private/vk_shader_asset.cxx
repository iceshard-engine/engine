/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "vk_shader_asset.hxx"

#include <ice/render/render_shader.hxx>
#include <ice/asset_type_archive.hxx>
#include <ice/asset.hxx>
#include <ice/path_utils.hxx>

namespace ice::render::vk
{

    auto asset_shader_state(
        void*,
        ice::AssetTypeDefinition const&,
        ice::Metadata const& metadata,
        ice::URI const& uri
    ) noexcept -> ice::AssetState
    {
        ice::String const ext = ice::path::extension(uri.path);
        if (ext == ".glsl" || ext == ".asl")
        {
            return AssetState::Raw;
        }
        return AssetState::Baked;
    }

    auto asset_shader_loader(
        void*,
        ice::Allocator& alloc,
        ice::AssetStorage&,
        ice::Metadata const& meta,
        ice::Data data,
        ice::Memory& out_data
    ) noexcept -> ice::Task<bool>
    {
        out_data = alloc.allocate(ice::meminfo_of<Data>);

        Data* shader_data = reinterpret_cast<Data*>(out_data.location);
        shader_data->location = data.location;
        shader_data->size = data.size;
        shader_data->alignment = data.alignment;
        co_return true;
    }

    void asset_type_shader_definition(ice::AssetTypeArchive& asset_type_archive) noexcept
    {
        static ice::String extensions[]{ ".asl", ".glsl", ".spv" };

        static ice::AssetTypeDefinition type_definition{
            .resource_extensions = extensions,
            .fn_asset_state = asset_shader_state,
            .fn_asset_loader = asset_shader_loader
        };

#if ISP_WINDOWS
        ice::ResourceCompiler const compiler{ VkShaderAssetModule::compiler_api };
        asset_type_archive.register_type(ice::render::AssetType_Shader, type_definition, &compiler);
#else
        asset_type_archive.register_type(ice::render::AssetType_Shader, type_definition, nullptr);
#endif
    }

    void VkShaderAssetModule::v1_archive_api(ice::detail::asset_system::v1::AssetTypeArchiveAPI& api) noexcept
    {
        api.register_types_fn = asset_type_shader_definition;
    }

    ice::api::resource_compiler::v1::ResourceCompilerAPI VkShaderAssetModule::compiler_api{};

} // namespace ice::render::vk
