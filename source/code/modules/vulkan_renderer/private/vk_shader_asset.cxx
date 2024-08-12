/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "vk_shader_asset.hxx"

#include <ice/render/render_shader.hxx>
#include <ice/asset_category_archive.hxx>
#include <ice/asset.hxx>
#include <ice/path_utils.hxx>

namespace ice::render::vk
{

    auto asset_shader_state(
        void*,
        ice::AssetCategoryDefinition const&,
        ice::Metadata const& metadata,
        ice::URI const& uri
    ) noexcept -> ice::AssetState
    {
        bool baked = false;
        if (ice::meta_read_bool(metadata, "ice.shader.baked"_sid, baked) && baked)
        {
            return AssetState::Baked;
        }

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

    void asset_category_shader_definition(
        ice::AssetCategoryArchive& asset_category_archive,
        ice::ModuleQuery const& module_query
    ) noexcept
    {
        static ice::String constexpr extensions[]{ ".asl", ".glsl", ".spv" };

        static ice::AssetCategoryDefinition definition{
            .resource_extensions = extensions,
            .fn_asset_state = asset_shader_state,
            .fn_asset_loader = asset_shader_loader
        };

#if ISP_WINDOWS
        ice::ResourceCompiler compiler{ };
        module_query.query_api(compiler);
        asset_category_archive.register_category(ice::render::AssetCategory_Shader, definition, &compiler);
#else
        asset_category_archive.register_category(ice::render::AssetCategory_Shader, definition, nullptr);
#endif
    }

    void VkShaderAssetModule::v1_archive_api(ice::detail::asset_system::v1::AssetArchiveAPI& api) noexcept
    {
        api.fn_register_categories = asset_category_shader_definition;
    }

} // namespace ice::render::vk
