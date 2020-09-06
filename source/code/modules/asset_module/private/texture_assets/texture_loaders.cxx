#include "texture_loaders.hxx"

#include <core/pod/hash.hxx>

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.hxx"

namespace iceshard
{

    StbTextureLoader::StbTextureLoader(core::allocator& alloc) noexcept
        : AssetLoader{ }
        , _allocator{ alloc }
        , _mesh_allocator{ _allocator }
        , _texture_status{ _allocator }
        , _texture_pixels{ _allocator }
    {
        core::pod::hash::reserve(_texture_status, 10);
        core::pod::hash::reserve(_texture_pixels, 10);
    }

    StbTextureLoader::~StbTextureLoader() noexcept
    {
    }

    auto StbTextureLoader::supported_asset_types() const noexcept -> core::pod::Array<asset::AssetType> const&
    {
        using asset::AssetType;

        static AssetType supported_types[]{
            AssetType::Texture
        };

        static core::pod::Array supported_types_view = [&]() noexcept
        {
            core::pod::Array<AssetType> array_view{ core::memory::globals::null_allocator() };
            core::pod::array::create_view(array_view, supported_types, core::size(supported_types));
            return array_view;
        }();

        return supported_types_view;
    }

    auto StbTextureLoader::request_asset(asset::Asset asset) noexcept -> asset::AssetStatus
    {
        return asset::AssetStatus::Invalid;
    }

    auto StbTextureLoader::load_asset(
        asset::Asset asset,
        resource::ResourceMetaView meta,
        core::data_view resource_data,
        asset::AssetData& result_data
    ) noexcept -> asset::AssetStatus
    {
        auto const* image_buffer_raw = reinterpret_cast<stbi_uc const*>(resource_data.data());

        int32_t meta_width;
        int32_t meta_height;
        int32_t meta_format;
        resource::get_meta_int32(meta, "texture.extents.width"_sid, meta_width);
        resource::get_meta_int32(meta, "texture.extents.height"_sid, meta_height);
        resource::get_meta_int32(meta, "texture.format"_sid, meta_format);

        switch (meta_format)
        {
        case 0:
        case 2:
        case 4:
            meta_format = 3;
            break;
        case 1:
        case 3:
            meta_format = 4;
            break;
        default:
            meta_format = 0;
        }


        int32_t width = 0;
        int32_t height = 0;
        int32_t channels = 0;

        auto* const image_buffer = stbi_load_from_memory(
            image_buffer_raw,
            resource_data.size(),
            &width, &height, &channels,
            STBI_rgb_alpha
        );

        IS_ASSERT(width == meta_width, "");
        IS_ASSERT(height == meta_height, "");

        /*if (channels != meta_format && image_buffer != nullptr)
        {
            stbi_image_free(image_buffer);
        }
        else */if (image_buffer != nullptr)
        {
            auto const asset_name = core::hash(asset.name);

            result_data.metadata = meta;
            result_data.content = core::data_view{ image_buffer, static_cast<uint32_t>(width * height * 4) };

            core::pod::hash::set(
                _texture_pixels,
                asset_name,
                reinterpret_cast<void*>(image_buffer)
            );

            core::pod::hash::set(
                _texture_status,
                asset_name,
                asset::AssetStatus::Loaded
            );

            return asset::AssetStatus::Loaded;
        }
        return asset::AssetStatus::Invalid;
    }

    void StbTextureLoader::release_asset(asset::Asset asset) noexcept
    {
        auto const asset_name_hash = core::hash(asset.name);
        auto const model_status = core::pod::hash::get(
            _texture_status,
            asset_name_hash,
            asset::AssetStatus::Invalid
        );

        if (model_status == asset::AssetStatus::Loaded)
        {
            void* const texture_pixels = core::pod::hash::get(_texture_pixels, asset_name_hash, nullptr);

            stbi_image_free(texture_pixels);

            core::pod::hash::remove(_texture_pixels, asset_name_hash);
            core::pod::hash::remove(_texture_status, asset_name_hash);
        }
    }

} // namespace iceshard
