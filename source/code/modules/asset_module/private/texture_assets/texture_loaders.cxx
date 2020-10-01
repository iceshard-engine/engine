#include "texture_loaders.hxx"

#include <core/memory.hxx>
#include <core/pod/hash.hxx>

namespace iceshard
{

    StbTextureLoader::StbTextureLoader(core::allocator& alloc) noexcept
        : AssetLoader{ }
        , _allocator{ alloc }
        , _texture_status{ _allocator }
        , _textures{ _allocator }
    {
        core::pod::hash::reserve(_texture_status, 10);
    }

    StbTextureLoader::~StbTextureLoader() noexcept
    {
    }

    auto StbTextureLoader::supported_asset_types() const noexcept -> core::pod::Array<asset::AssetType> const&
    {
        static asset::AssetType supported_types[]{
            asset::AssetType::Texture
        };
        static core::pod::Array supported_types_view = core::pod::array::create_view(supported_types);
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
        using iceshard::renderer::data::Texture;

        auto model_status = core::pod::hash::get(
            _texture_status,
            core::hash(asset.name),
            asset::AssetStatus::Requested
        );

        if (model_status == asset::AssetStatus::Invalid)
        {
            return model_status;
        }

        if (model_status == asset::AssetStatus::Requested)
        {
            auto const asset_name = core::hash(asset.name);

            Texture texture = *reinterpret_cast<Texture const*>(resource_data.data());
            texture.data = core::memory::utils::pointer_add(
                resource_data.data(),
                static_cast<uint32_t>(reinterpret_cast<uintptr_t>(texture.data))
            );

            model_status = asset::AssetStatus::Loaded;

            core::pod::hash::set(
                _textures,
                asset_name,
                texture
            );

            core::pod::hash::set(
                _texture_status,
                asset_name,
                model_status
            );
        }

        static Texture const empty_texture{ };

        result_data.metadata = meta;
        result_data.content = {
            std::addressof(
                core::pod::hash::get(
                    _textures,
                    core::hash(asset.name),
                    empty_texture
                )
            ),
            sizeof(Texture)
        };

        return model_status;
    }

    bool StbTextureLoader::release_asset(asset::Asset asset) noexcept
    {
        auto const asset_name_hash = core::hash(asset.name);
        auto const model_status = core::pod::hash::get(
            _texture_status,
            asset_name_hash,
            asset::AssetStatus::Invalid
        );

        if (model_status == asset::AssetStatus::Loaded)
        {
            core::pod::hash::remove(_textures, asset_name_hash);
            core::pod::hash::remove(_texture_status, asset_name_hash);
            return true;
        }
        return false;
    }

} // namespace iceshard
