#include <asset_system/assets/asset_shader.hxx>
#include <core/pod/array.hxx>

namespace asset::detail
{

    class AssetShaderResolver : public asset::AssetResolver
    {
    public:
        auto resolve_asset_type(core::StringView extension, resource::ResourceMetaView const& meta) noexcept -> AssetType override
        {
            AssetType result = AssetType::Unresolved;
            if (core::string::equals(extension, ".glsl"))
            {
                result = AssetType::Shader;
            }
            else if (core::string::equals(extension, ".spv"))
            {
                result = AssetType::Shader;
            }
            else
            {
                int32_t shader_type = 0;
                if (resource::get_meta_int32(meta, "shader.type"_sid, shader_type))
                {
                    result = (shader_type == 'v') || (shader_type == 'f') ? AssetType::Shader : AssetType::Unresolved;
                }
            }
            return result;
        }
    };

    class AssetShaderLoader : public asset::AssetLoader
    {
    public:
        auto request_asset([[maybe_unused]] asset::Asset asset_reference) noexcept -> asset::AssetStatus override
        {
            return asset::AssetStatus::Invalid;
        }

        auto load_asset(
            asset::Asset /* asset */,
            resource::ResourceMetaView meta,
            core::data_view resource_data,
            asset::AssetData& result_data
        ) noexcept -> asset::AssetStatus override
        {
            result_data.content = resource_data;
            memcpy(&result_data.metadata, &meta, sizeof(resource::ResourceMetaView));
            return asset::AssetStatus::Loaded;
        }

        void release_asset([[maybe_unused]] asset::Asset asset_reference) noexcept override
        {
        }
    };

}

auto asset::default_resolver_shader(core::allocator& alloc) noexcept -> core::memory::unique_pointer<asset::AssetResolver>
{
    return core::memory::make_unique<asset::AssetResolver, asset::detail::AssetShaderResolver>(alloc);
}

auto asset::default_loader_shader(core::allocator& alloc) noexcept -> core::memory::unique_pointer<asset::AssetLoader>
{
    return core::memory::make_unique<asset::AssetLoader, asset::detail::AssetShaderLoader>(alloc);
}
