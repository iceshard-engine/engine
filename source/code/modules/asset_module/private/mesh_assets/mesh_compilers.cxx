#include "mesh_compilers.hxx"
#include <iceshard/renderer/render_model.hxx>
#include <core/pod/array.hxx>
#include <core/memory.hxx>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace iceshard
{

    namespace detail
    {

        using iceshard::renderer::data::Mesh;
        using iceshard::renderer::data::Model;
        using iceshard::renderer::api::v1_1::data::Vertice;

        struct MutableModel
        {
            uint32_t mesh_count;
            uint32_t vertice_data_size;
            uint32_t indice_data_size;
            Mesh* mesh_list;
            Vertice* vertice_data;
            core::math::u16* indice_data;
        };

        void process_mesh(
            MutableModel& model,
            Mesh& model_mesh,
            aiMesh* mesh,
            aiMatrix4x4 const& mtx
        ) noexcept
        {
            using core::math::vec2f;
            using core::math::vec3f;
            using core::math::u16;

            {
                Vertice* vertice_data = model.vertice_data + model_mesh.vertice_offset;
                Vertice const* const vertice_data_end = vertice_data + model_mesh.vertice_count;

                auto uvs_array = mesh->mTextureCoords[0];
                for (uint32_t i = 0; i < model_mesh.vertice_count; ++i)
                {
                    auto vec = mesh->mVertices[i];
                    auto uvs = uvs_array[i];
                    auto norm = mesh->mNormals[i];

                    Vertice& v = *vertice_data;
                    v.pos = {
                        vec.x,
                        vec.y,
                        vec.z,
                    };
                    v.norm = {
                        norm.x,
                        norm.y,
                        norm.z,
                    };
                    v.uv = {
                        uvs.x,
                        uvs.y,
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
            MutableModel& model,
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
                    model,
                    model.mesh_list[mesh_idx],
                    scene->mMeshes[mesh_idx],
                    node->mTransformation * mtx
                );
            }

            for (uint32_t i = 0; i < node->mNumChildren; ++i)
            {
                process_node(
                    model,
                    node->mChildren[i],
                    scene,
                    node->mTransformation * mtx
                );
            }
        }

    } // namespace detail

    auto AssimpMeshCompiler::supported_asset_types() const noexcept -> core::pod::Array<asset::AssetType> const&
    {
        static asset::AssetType supported_types[] = {
            asset::AssetType::Mesh
        };

        static auto supported_types_array = core::pod::array::create_view(supported_types);
        return supported_types_array;
    }

    auto AssimpMeshCompiler::compile_asset(
        core::allocator& alloc,
        resource::ResourceSystem& resource_system,
        asset::Asset asset,
        core::data_view resource_data,
        resource::ResourceMetaView const& meta,
        asset::AssetCompilationResult& result_out
    ) noexcept -> asset::AssetCompilationStatus
    {
        if (asset.type != asset::AssetType::Mesh)
        {
            return asset::AssetCompilationStatus::Failed;
        }

        using iceshard::renderer::data::Model;
        using iceshard::renderer::data::Mesh;
        using iceshard::renderer::api::v1_1::data::Vertice;

        Assimp::Importer importer;
        //importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 80);

        aiScene const* scene = nullptr;

        core::StringView hint = "";
        if (resource::get_meta_string(meta, "model.assimp.hint"_sid, hint))
        {
            scene = importer.ReadFileFromMemory(
                resource_data.data(),
                resource_data.size(),
                aiProcessPreset_TargetRealtime_Fast,
                hint.data()
            );
        }
        else
        {
            scene = importer.ReadFileFromMemory(
                resource_data.data(),
                resource_data.size(),
                aiProcessPreset_TargetRealtime_Fast
            );
        }


        // We don't know this file format
        if (scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || scene->mRootNode == nullptr)
        {
            return asset::AssetCompilationStatus::Failed_InvalidData;
        }

        if (scene->mNumMeshes == 0)
        {
            return asset::AssetCompilationStatus::Failed_InvalidData;
        }

        core::buffer::clear(result_out.data);

        {
            uint32_t vertice_offset = 0;
            uint32_t indice_offset = 0;

            for (uint32_t mesh_idx = 0; mesh_idx < scene->mNumMeshes; ++mesh_idx)
            {
                aiMesh const* const scene_mesh = scene->mMeshes[mesh_idx];
                vertice_offset += scene_mesh->mNumVertices;
                indice_offset += scene_mesh->mNumFaces * 3;
            }

            uint32_t const vertice_data_size = vertice_offset * sizeof(Vertice);
            uint32_t const indice_data_size = indice_offset * sizeof(core::math::u16);

            uint32_t mesh_data_size = 0;
            mesh_data_size += sizeof(Model);
            mesh_data_size += sizeof(Mesh) * scene->mNumMeshes;
            mesh_data_size += vertice_data_size;
            mesh_data_size += indice_data_size;
            mesh_data_size += alignof(core::math::vec3f) * 2 * scene->mNumMeshes;

            core::buffer::reserve(result_out.data, mesh_data_size + core::buffer::size(result_out.data));

            void* model_ptr = core::buffer::append_aligned(
                result_out.data,
                { nullptr, sizeof(Model), alignof(Model) }
            );

            void* mesh_ptr = core::buffer::append_aligned(
                result_out.data,
                { nullptr, sizeof(Mesh) * scene->mNumMeshes, alignof(Mesh) }
            );

            Model& model = *reinterpret_cast<Model*>(model_ptr);
            model.mesh_count = scene->mNumMeshes;
            model.mesh_list = reinterpret_cast<Mesh*>(mesh_ptr);
            model.vertice_data = nullptr;
            model.vertice_data_size = vertice_data_size;
            model.indice_data = nullptr;
            model.indice_data_size = indice_data_size;

            detail::MutableModel mutable_model;
            mutable_model.mesh_count = scene->mNumMeshes;
            mutable_model.mesh_list = reinterpret_cast<Mesh*>(mesh_ptr);
            mutable_model.vertice_data = nullptr;
            mutable_model.vertice_data_size = vertice_data_size;
            mutable_model.indice_data = nullptr;
            mutable_model.indice_data_size = indice_data_size;

            vertice_offset = 0;
            indice_offset = 0;

            for (uint32_t mesh_idx = 0; mesh_idx < model.mesh_count; ++mesh_idx)
            {
                aiMesh const* const scene_mesh = scene->mMeshes[mesh_idx];
                Mesh& model_mesh = mutable_model.mesh_list[mesh_idx];

                model_mesh.vertice_count = scene_mesh->mNumVertices;
                model_mesh.vertice_offset = vertice_offset;
                vertice_offset += model_mesh.vertice_count;

                model_mesh.indice_count = scene_mesh->mNumFaces * 3;
                model_mesh.indice_offset = indice_offset;
                indice_offset += model_mesh.indice_count;
            }

            auto* vertice_ptr = core::buffer::append_aligned(
                result_out.data,
                { nullptr, model.vertice_data_size, alignof(Vertice) }
            );
            auto* indice_ptr = core::buffer::append_aligned(
                result_out.data,
                { nullptr, model.indice_data_size, alignof(core::math::u16) }
            );

            model.vertice_data = reinterpret_cast<Vertice*>(vertice_ptr);
            model.indice_data = reinterpret_cast<core::math::u16*>(indice_ptr);

            detail::process_node(
                mutable_model,
                scene->mRootNode,
                scene,
                aiMatrix4x4{ }
            );

            auto* const data_begin = core::buffer::data(result_out.data);

            model.mesh_list = reinterpret_cast<Mesh*>(
                static_cast<uintptr_t>(
                    core::memory::utils::pointer_distance(data_begin, mesh_ptr)
                )
            );

            model.vertice_data = reinterpret_cast<Vertice*>(
                static_cast<uintptr_t>(
                    core::memory::utils::pointer_distance(data_begin, vertice_ptr)
                )
            );

            model.indice_data = reinterpret_cast<core::math::u16*>(
                static_cast<uintptr_t>(
                    core::memory::utils::pointer_distance(data_begin, indice_ptr)
                )
            );

            resource::copy_meta(result_out.metadata, meta);
            resource::set_meta_int32(result_out.metadata, "asset.type"_sid, static_cast<int32_t>(asset.type));
        }

        return asset::AssetCompilationStatus::Success;
    }

} // namespace iceshard
