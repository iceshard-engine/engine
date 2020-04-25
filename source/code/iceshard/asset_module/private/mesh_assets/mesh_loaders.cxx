#include "mesh_loaders.hxx"

#include <core/memory.hxx>
#include <core/pod/array.hxx>
#include <core/data/buffer.hxx>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace iceshard
{

    namespace detail
    {

        void process_mesh(core::allocator& alloc, asset::v1::Mesh& out, aiMesh* mesh, aiScene const* scene) noexcept
        {
            out.vertice_count = mesh->mNumVertices;
            out.indice_count = mesh->mNumFaces * 3;

            out.vertice_data = (float*) alloc.allocate(out.vertice_count * sizeof(float) * 6);
            out.indice_data = (uint16_t*) alloc.allocate(out.indice_count * sizeof(uint16_t));

            float* vertice_data_it = out.vertice_data;
            auto const* const vertice_data_end = vertice_data_it + (out.vertice_count * 6);

            for (uint32_t i = 0; i < out.vertice_count; ++i)
            {
                *vertice_data_it++ = mesh->mVertices[i].x;
                *vertice_data_it++ = mesh->mVertices[i].y;
                *vertice_data_it++ = mesh->mVertices[i].z;
                *vertice_data_it++ = 0.3;
                *vertice_data_it++ = 0.6;
                *vertice_data_it++ = 0.2;
            }

            IS_ASSERT(vertice_data_it == vertice_data_end, "Mesh vertice loading error!");

            uint16_t* indice_data_it = out.indice_data;
            auto const* const indice_data_end = indice_data_it + out.indice_count;

            for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
            {
                aiFace* face = mesh->mFaces + i;

                for (uint32_t fi = 0; fi < face->mNumIndices; ++fi)
                {
                    *indice_data_it++ = face->mIndices[fi];
                }
            }

            IS_ASSERT(indice_data_it == indice_data_end, "Mesh indice loading error!");
        }

        void process_node(core::allocator& alloc, asset::v1::Mesh& mesh, aiNode* node, aiScene const* scene) noexcept
        {
            if (node->mNumMeshes >= 1)
            {
                process_mesh(alloc, mesh, scene->mMeshes[node->mMeshes[0]], scene);
            }

            for (uint32_t i = 0; i < node->mNumChildren; ++i)
            {
                process_node(alloc, mesh, node->mChildren[i], scene);
            }
            // Resursive for children
        }

    } // namespace detail

    AssimpMeshLoader::AssimpMeshLoader(core::allocator& alloc) noexcept
        : _allocator{ alloc }
        , _mesh_allocator{ _allocator }
        , _meshes{ _allocator }
    {
        core::pod::array::reserve(_meshes, 1);
    }

    AssimpMeshLoader::~AssimpMeshLoader() noexcept
    {
        for (auto const& mesh : _meshes)
        {
            _mesh_allocator.deallocate(mesh.indice_data);
            _mesh_allocator.deallocate(mesh.vertice_data);
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

        asset::v1::Mesh mesh{ };

        detail::process_node(
            _mesh_allocator,
            mesh,
            scene->mRootNode,
            scene
        );

        if (mesh.indice_count == 0 || mesh.vertice_count == 0)
        {
            return asset::AssetStatus::Invalid;
        }

        core::pod::array::push_back(_meshes, mesh);
        result_data.metadata = meta;
        result_data.content = {
            std::addressof(core::pod::array::back(_meshes)),
            sizeof(mesh)
        };
        return asset::AssetStatus::Loaded;
    }

    void AssimpMeshLoader::release_asset(asset::Asset asset) noexcept
    {
    }

} // namespace iceshard