#include "asset_shader.hxx"
#include <ice/asset.hxx>
#include <ice/asset_type_archive.hxx>
#include <ice/render/render_shader.hxx>

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

    auto asset_shader_loader(
        void*,
        ice::Allocator& alloc,
        ice::AssetStorage&,
        ice::Metadata const& meta,
        ice::Data data,
        ice::Memory& out_data
    ) noexcept -> ice::Task<bool>
    {
        out_data.size = sizeof(Data);
        out_data.alignment = alignof(Data);
        out_data.location = alloc.allocate(out_data.size, out_data.alignment);

        Data* shader_data = reinterpret_cast<Data*>(out_data.location);
        shader_data->location = data.location;
        shader_data->size = data.size;
        shader_data->alignment = data.alignment;
        co_return true;
    }

    void asset_type_shader_definition(ice::AssetTypeArchive& asset_type_archive) noexcept
    {
        static ice::Utf8String extensions[]{ u8".spv" };

        static ice::AssetTypeDefinition type_definition{
            .resource_extensions = extensions,
            .fn_asset_state = asset_shader_state,
            .fn_asset_loader = asset_shader_loader
        };

        asset_type_archive.register_type(ice::render::AssetType_Shader, type_definition);
    }

} // namespace ice
