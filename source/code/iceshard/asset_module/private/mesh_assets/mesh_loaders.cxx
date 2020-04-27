#include "mesh_loaders.hxx"

#include <core/memory.hxx>
#include <core/pod/array.hxx>
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
            aiMesh* mesh
        ) noexcept
        {
            using core::math::u16;
            using core::math::vec3;

            {
                vec3* vertice_data = model.vertice_data + model_mesh.vertice_offset;
                vec3 const* const vertice_data_end = vertice_data + (model_mesh.vertice_count * 2);

                for (uint32_t i = 0; i < model_mesh.vertice_count; ++i)
                {
                    *vertice_data = {
                        mesh->mVertices[i].x,
                        mesh->mVertices[i].y,
                        mesh->mVertices[i].z
                    };
                    vertice_data += 1;
                    *vertice_data = {
                        0.3,
                        0.6,
                        0.2,
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
            aiScene const* scene
        ) noexcept
        {
            if (node->mNumMeshes >= 1)
            {
                auto const mesh_idx = node->mMeshes[0];
                process_mesh(
                    alloc,
                    model,
                    model.mesh_list[mesh_idx],
                    scene->mMeshes[node->mMeshes[0]]
                );
            }

            for (uint32_t i = 0; i < node->mNumChildren; ++i)
            {
                process_node(
                    alloc,
                    model,
                    node->mChildren[i],
                    scene
                );
            }
        }

    } // namespace detail

    AssimpMeshLoader::AssimpMeshLoader(core::allocator& alloc) noexcept
        : _allocator{ alloc }
        , _mesh_allocator{ _allocator }
        , _models{ _allocator }
    {
        core::pod::array::reserve(_models, 1);
    }

    AssimpMeshLoader::~AssimpMeshLoader() noexcept
    {
        for (auto const& model : _models)
        {
            _mesh_allocator.deallocate(model.mesh_list);
            _mesh_allocator.deallocate(model.vertice_data);
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
        Assimp::Importer importer;

        aiScene const* scene = importer.ReadFileFromMemory(
            resource_data.data(),
            resource_data.size(),
            aiProcess_Triangulate | aiProcess_FlipUVs
        );

        // We don't know the this file format
        if (scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || scene->mRootNode == nullptr)
        {
            return asset::AssetStatus::Invalid;
        }

        using iceshard::renderer::v1::Model;
        using iceshard::renderer::v1::Mesh;

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

            model.vertice_data_size = vertice_offset * sizeof(core::math::vec3) * 2;
            model.indice_data_size = indice_offset * sizeof(core::math::u16);

            uint32_t mesh_data_size = 0;
            mesh_data_size += model.vertice_data_size;
            mesh_data_size += model.indice_data_size;
            mesh_data_size += alignof(core::math::vec3) * 2 * model.mesh_count;

            void* mesh_data = _mesh_allocator.allocate(mesh_data_size);

            model.vertice_data = reinterpret_cast<core::math::vec3*>(mesh_data);
            model.indice_data = reinterpret_cast<core::math::u16*>(
                core::memory::utils::pointer_add(mesh_data, model.vertice_data_size)
            );
        }

        uint32_t offset = 0;
        detail::process_node(
            _mesh_allocator,
            model,
            scene->mRootNode,
            scene
        );

        core::pod::array::push_back(_models, model);
        result_data.metadata = meta;
        result_data.content = {
            std::addressof(core::pod::array::back(_models)),
            sizeof(model)
        };
        return asset::AssetStatus::Loaded;
    }

    void AssimpMeshLoader::release_asset(asset::Asset asset) noexcept
    {
    }

} // namespace iceshard