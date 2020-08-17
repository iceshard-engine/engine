#include "mesh_loaders.hxx"

#include <core/memory.hxx>
#include <core/pod/hash.hxx>
#include <core/data/buffer.hxx>

#include <iceshard/renderer/render_model.hxx>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


namespace iceshard
{

    namespace detail
    {

        using iceshard::renderer::v1::Mesh;
        using iceshard::renderer::v1::Model;

        void process_mesh(
            core::allocator& alloc,
            Model& model,
            Mesh& model_mesh,
            aiMesh* mesh,
            aiMatrix4x4 const& mtx
        ) noexcept
        {
            using core::math::vec3f;
            using core::math::u16;

            {
                vec3f* vertice_data = model.vertice_data + model_mesh.vertice_offset * 2llu;
                vec3f const* const vertice_data_end = vertice_data + model_mesh.vertice_count * 2llu;

                for (uint32_t i = 0; i < model_mesh.vertice_count; ++i)
                {
                    auto vec = mesh->mVertices[i];
                    auto norm = mesh->mNormals[i];

                    *vertice_data = {
                        vec.x,
                        vec.y,
                        vec.z
                    };
                    vertice_data += 1;

                    *vertice_data = {
                        norm.x,
                        norm.y,
                        norm.z
                    };
                    vertice_data += 1;
                }

                IS_ASSERT(vertice_data == vertice_data_end, "Mesh vertice loading error!");
            }

            {
                u16* indice_data = model.indice_data + model_mesh.indice_offset;
                u16 const* const indice_data_end = indice_data + model_mesh.indice_count;

                for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
                {
                    aiFace* face = mesh->mFaces + i;

                    for (uint32_t fi = 0; fi < face->mNumIndices; ++fi)
                    {
                        *indice_data++ = face->mIndices[fi];
                    }
                }

                IS_ASSERT(indice_data == indice_data_end, "Mesh indice loading error!");
            }
        }

        void process_node(
            core::allocator& alloc,
            Model& model,
            aiNode const* node,
            aiScene const* scene,
            aiMatrix4x4 mtx
        ) noexcept
        {
            for (uint32_t i = 0; i < node->mNumMeshes; ++i)
            {
                auto const mesh_idx = node->mMeshes[i];
                model.mesh_list[mesh_idx].local_xform = core::math::transpose(
                    core::math::matrix_from_data(
                        node->mTransformation * mtx
                    )
                );

                process_mesh(
                    alloc,
                    model,
                    model.mesh_list[mesh_idx],
                    scene->mMeshes[mesh_idx],
                    node->mTransformation * mtx
                );
            }

            for (uint32_t i = 0; i < node->mNumChildren; ++i)
            {
                process_node(
                    alloc,
                    model,
                    node->mChildren[i],
                    scene,
                    node->mTransformation * mtx
                );
            }
        }

    } // namespace detail

    AssimpMeshLoader::AssimpMeshLoader(core::allocator& alloc) noexcept
        : _allocator{ alloc }
        , _mesh_allocator{ _allocator }
        , _models_status{ _allocator }
        , _models{ _allocator }
    {
        core::pod::hash::reserve(_models_status, 10);
        core::pod::hash::reserve(_models, 10);
    }

    AssimpMeshLoader::~AssimpMeshLoader() noexcept
    {
        for (auto const& model : _models)
        {
            _mesh_allocator.deallocate(model.value.mesh_list);
            _mesh_allocator.deallocate(model.value.vertice_data);
        }
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
        using iceshard::renderer::v1::Model;
        using iceshard::renderer::v1::Mesh;

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
            Assimp::Importer importer;

            importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 80);
            aiScene const* scene = importer.ReadFileFromMemory(
                resource_data.data(),
                resource_data.size(),
                aiProcessPreset_TargetRealtime_Fast
            );

            // We don't know this file format
            if (scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || scene->mRootNode == nullptr)
            {
                return asset::AssetStatus::Invalid;
            }

            if (scene->mNumMeshes == 0)
            {
                return asset::AssetStatus::Invalid;
            }

            Model model{
                .mesh_count = scene->mNumMeshes,
                .mesh_list = (Mesh*)_mesh_allocator.allocate(sizeof(Mesh) * scene->mNumMeshes),
                .vertice_data = nullptr,
                .indice_data = nullptr,
            };

            {
                uint32_t vertice_offset = 0;
                uint32_t indice_offset = 0;

                for (uint32_t mesh_idx = 0; mesh_idx < model.mesh_count; ++mesh_idx)
                {
                    aiMesh const* const scene_mesh = scene->mMeshes[mesh_idx];
                    Mesh& model_mesh = model.mesh_list[mesh_idx];

                    model_mesh.vertice_count = scene_mesh->mNumVertices;
                    model_mesh.vertice_offset = vertice_offset;
                    vertice_offset += model_mesh.vertice_count;

                    model_mesh.indice_count = scene_mesh->mNumFaces * 3;
                    model_mesh.indice_offset = indice_offset;
                    indice_offset += model_mesh.indice_count;
                }

                model.vertice_data_size = vertice_offset * sizeof(core::math::vec3f) * 2;
                model.indice_data_size = indice_offset * sizeof(core::math::u16);

                uint32_t mesh_data_size = 0;
                mesh_data_size += model.vertice_data_size;
                mesh_data_size += model.indice_data_size;
                mesh_data_size += alignof(core::math::vec3f) * 2 * model.mesh_count;

                void* mesh_data = _mesh_allocator.allocate(mesh_data_size);

                model.vertice_data = reinterpret_cast<core::math::vec3f*>(mesh_data);
                model.indice_data = reinterpret_cast<core::math::u16*>(
                    core::memory::utils::pointer_add(mesh_data, model.vertice_data_size)
                );
            }

            uint32_t offset = 0;
            detail::process_node(
                _mesh_allocator,
                model,
                scene->mRootNode,
                scene,
                aiMatrix4x4{ }
            );

            core::pod::hash::set(
                _models,
                core::hash(asset.name),
                model
            );

            core::pod::hash::set(
                _models_status,
                core::hash(asset.name),
                asset::AssetStatus::Loaded
            );

            model_status = asset::AssetStatus::Loaded;
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

    void AssimpMeshLoader::release_asset(asset::Asset asset) noexcept
    {
        using iceshard::renderer::v1::Model;

        auto const asset_name_hash = core::hash(asset.name);
        auto const model_status = core::pod::hash::get(
            _models_status,
            asset_name_hash,
            asset::AssetStatus::Invalid
        );

        if (model_status == asset::AssetStatus::Loaded)
        {
            static Model const empty_model{ };
            Model const& model = core::pod::hash::get(_models, asset_name_hash, empty_model);

            _mesh_allocator.deallocate(model.mesh_list);
            _mesh_allocator.deallocate(model.vertice_data);

            core::pod::hash::remove(_models, asset_name_hash);
            core::pod::hash::remove(_models_status, asset_name_hash);
        }
    }

} // namespace iceshard
