#include "mesh_loaders.hxx"

#include <core/memory.hxx>
#include <core/pod/hash.hxx>
#include <core/data/buffer.hxx>

#include <iceshard/renderer/render_model.hxx>

#include <resource/resource_meta.hxx>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


namespace iceshard
{

    AssimpMeshLoader::AssimpMeshLoader(core::allocator& alloc) noexcept
        : _allocator{ alloc }
        , _models_status{ _allocator }
        , _models{ _allocator }
    {
        core::pod::hash::reserve(_models_status, 10);
        core::pod::hash::reserve(_models, 10);
    }

    AssimpMeshLoader::~AssimpMeshLoader() noexcept
    {
    }

    auto AssimpMeshLoader::supported_asset_types() const noexcept -> core::pod::Array<asset::AssetType> const&
    {
        using asset::AssetType;

        static AssetType supported_types[]{
            AssetType::Mesh
        };

        static core::pod::Array supported_types_view = [&]() noexcept
        {
            core::pod::Array<AssetType> array_view{ core::memory::globals::null_allocator() };
            core::pod::array::create_view(array_view, supported_types, core::size(supported_types));
            return array_view;
        }();

        return supported_types_view;
    }

    auto AssimpMeshLoader::request_asset(asset::Asset asset) noexcept -> asset::AssetStatus
    {
        return asset::AssetStatus::Invalid;
    }

    auto AssimpMeshLoader::load_asset(
        asset::Asset asset,
        resource::ResourceMetaView meta,
        core::data_view resource_data,
        asset::AssetData& result_data
    ) noexcept -> asset::AssetStatus
    {
        using iceshard::renderer::api::v1_1::data::Vertice;
        using iceshard::renderer::data::Model;
        using iceshard::renderer::data::Mesh;

        auto model_status = core::pod::hash::get(
            _models_status,
            core::hash(asset.name),
            asset::AssetStatus::Requested
        );

        if (model_status == asset::AssetStatus::Invalid)
        {
            return model_status;
        }

        if (model_status == asset::AssetStatus::Requested)
        {
            Model model_data = *reinterpret_cast<Model const*>(resource_data.data());

            void const* mesh_ptr = core::memory::utils::pointer_add(
                resource_data.data(),
                static_cast<uint32_t>(reinterpret_cast<uintptr_t>(model_data.mesh_list))
            );
            void const* vertice_ptr = core::memory::utils::pointer_add(
                resource_data.data(),
                static_cast<uint32_t>(reinterpret_cast<uintptr_t>(model_data.vertice_data))
            );
            void const* indice_ptr = core::memory::utils::pointer_add(
                resource_data.data(),
                static_cast<uint32_t>(reinterpret_cast<uintptr_t>(model_data.indice_data))
            );

            Model model_view{
                .mesh_count = model_data.mesh_count,
                .vertice_data_size = model_data.vertice_data_size,
                .indice_data_size = model_data.indice_data_size,
                .mesh_list = reinterpret_cast<Mesh const*>(mesh_ptr),
                .vertice_data = reinterpret_cast<Vertice const*>(vertice_ptr),
                .indice_data = reinterpret_cast<core::math::u16 const*>(indice_ptr),
            };

            model_status = asset::AssetStatus::Loaded;

            core::pod::hash::set(_models, core::hash(asset.name), model_view);
            core::pod::hash::set(_models_status, core::hash(asset.name), model_status);
        }

        static Model const empty_model{ };

        result_data.metadata = meta;
        result_data.content = {
            std::addressof(
                core::pod::hash::get(
                    _models,
                    core::hash(asset.name),
                    empty_model
                )
            ),
            sizeof(Model)
        };

        return model_status;
    }

    bool AssimpMeshLoader::release_asset(asset::Asset asset) noexcept
    {
        using iceshard::renderer::data::Model;

        auto const asset_name_hash = core::hash(asset.name);
        auto const model_status = core::pod::hash::get(
            _models_status,
            asset_name_hash,
            asset::AssetStatus::Invalid
        );

        if (model_status == asset::AssetStatus::Loaded)
        {
            core::pod::hash::remove(_models, asset_name_hash);
            core::pod::hash::remove(_models_status, asset_name_hash);
            return true;
        }

        return false;
    }

} // namespace iceshard
