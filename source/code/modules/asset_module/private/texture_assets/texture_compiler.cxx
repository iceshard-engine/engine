#include "texture_compiler.hxx"
#include <core/memory.hxx>
#include <core/pod/array.hxx>
#include <iceshard/renderer/render_model.hxx>


#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.hxx"

namespace iceshard
{

    using iceshard::renderer::api::v1_1::data::Texture;

    auto StbTextureCompiler::supported_asset_types() const noexcept -> core::pod::Array<asset::AssetType> const&
    {
        static asset::AssetType supported_types[] = {
            asset::AssetType::Texture
        };
        static auto supported_types_array = core::pod::array::create_view(supported_types);
        return supported_types_array;
    }

    auto StbTextureCompiler::compile_asset(
        core::allocator& alloc,
        resource::ResourceSystem& resource_system,
        asset::Asset asset,
        core::data_view resource_data,
        resource::ResourceMetaView const& asset_meta,
        asset::AssetCompilationResult& result_out
    ) noexcept -> asset::AssetCompilationStatus
    {
        auto const* image_buffer_raw = reinterpret_cast<stbi_uc const*>(resource_data.data());

        int32_t width = 0;
        int32_t height = 0;
        int32_t channels = 0;

        auto* const image_buffer = stbi_load_from_memory(
            image_buffer_raw,
            resource_data.size(),
            &width, &height, &channels,
            STBI_rgb_alpha
        );

        if (image_buffer != nullptr)
        {
            auto image_data = core::data_view{
                image_buffer,
                width * height * 4 * sizeof(stbi_uc)
            };

            core::buffer::clear(result_out.data);
            core::buffer::reserve(result_out.data, sizeof(iceshard::renderer::data::Texture) + image_data.size() + 8);

            void* texture_ptr = core::buffer::append(
                result_out.data,
                core::data_view{ nullptr, sizeof(iceshard::renderer::data::Texture) }
            );

            void* data_ptr = core::buffer::append(
                result_out.data,
                image_data
            );

            auto* texture = reinterpret_cast<iceshard::renderer::data::Texture*>(texture_ptr);
            texture->width = width;
            texture->height = height;
            texture->format = iceshard::renderer::api::TextureFormat::UnormRGBA;
            texture->data = reinterpret_cast<void const*>(
                static_cast<uintptr_t>(
                    core::memory::utils::pointer_distance(texture_ptr, data_ptr)
                )
            );

            resource::copy_meta(result_out.metadata, asset_meta);
            return asset::AssetCompilationStatus::Success;
        }
        return asset::AssetCompilationStatus::Failed_InvalidData;
    }

} // namespace iceshard
