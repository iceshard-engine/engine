/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "asset_mesh.hxx"

#include <ice/asset.hxx>
#include <ice/render/render_buffer.hxx>

namespace ice
{

    static auto asset_mesh_state(
        void*,
        ice::AssetCategoryDefinition const&,
        ice::Config const&,
        ice::URI const& uri
    ) noexcept -> ice::AssetState
    {
        return AssetState::Baked;
    }

    static auto asset_mesh_loader(
        void*,
        ice::Allocator& alloc,
        ice::AssetStorage&,
        ice::Config const& meta,
        ice::Data data,
        ice::Memory& out_memory
    ) noexcept -> ice::Task<bool>
    {
        // For Baked meshes the Loaded state is the same as the baked state.
        // Returning 'true' allows the AssetStorage to promote the baked data to Loaded state.
        co_return true;
    }

    void asset_category_mesh_definition(
        ice::AssetCategoryArchive& asset_category_archive,
        ice::ModuleQuery const& module_query
    ) noexcept
    {
        static ice::String extensions[]{ ".ismf" };

        // static ice::ResourceCompiler const compiler{
        //     .fn_compile_source = nullptr,
        // };

        static ice::AssetCategoryDefinition const definition{
            .resource_extensions = extensions,
            .fn_asset_state = asset_mesh_state,
            .fn_asset_loader = asset_mesh_loader,
        };

        asset_category_archive.register_category(ice::render::AssetCategory_Mesh, definition, nullptr);
    }

} // namespace ice