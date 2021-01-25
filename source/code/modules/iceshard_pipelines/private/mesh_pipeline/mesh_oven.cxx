#include "mesh_oven.hxx"
#include <ice/render/render_model.hxx>
#include <ice/assert.hxx>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace ice
{

    namespace detail
    {

        struct MutableModel
        {
            ice::u32 mesh_count;
            ice::u32 vertice_data_size;
            ice::u32 indice_data_size;
            ice::render::Mesh* mesh_list;
            ice::render::Vertice* vertice_data;
            ice::u16* indice_data;
        };

        void process_mesh(
            ice::detail::MutableModel& model,
            ice::render::Mesh& model_mesh,
            aiMesh* mesh,
            aiMatrix4x4 const& mtx
        ) noexcept
        {
            using ice::u16;
            using ice::math::vec2f;
            using ice::math::vec3f;

            {
                ice::render::Vertice* vertice_data = model.vertice_data + model_mesh.vertice_offset;
                ice::render::Vertice const* const vertice_data_end = vertice_data + model_mesh.vertice_count;

                aiVector3D* uvs_array = mesh->mTextureCoords[0];
                for (ice::u32 i = 0; i < model_mesh.vertice_count; ++i)
                {
                    aiVector3D& vec = mesh->mVertices[i];
                    aiVector3D& uvs = uvs_array[i];
                    aiVector3D& norm = mesh->mNormals[i];

                    ice::render::Vertice& v = *vertice_data;
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

                ICE_ASSERT(vertice_data == vertice_data_end, "Mesh vertice loading error!");
            }

            {
                ice::u16* indice_data = model.indice_data + model_mesh.indice_offset;
                ice::u16 const* const indice_data_end = indice_data + model_mesh.indice_count;

                for (ice::u32 i = 0; i < mesh->mNumFaces; ++i)
                {
                    aiFace* face = mesh->mFaces + i;

                    for (ice::u32 fi = 0; fi < face->mNumIndices; ++fi)
                    {
                        *indice_data++ = face->mIndices[fi];
                    }
                }

                ICE_ASSERT(indice_data == indice_data_end, "Mesh indice loading error!");
            }
        }

        void process_node(
            ice::detail::MutableModel& model,
            aiNode const* node,
            aiScene const* scene,
            aiMatrix4x4 const& mtx
        ) noexcept
        {
            for (ice::u32 i = 0; i < node->mNumMeshes; ++i)
            {
                ice::u32 const mesh_idx = node->mMeshes[i];
                aiMatrix4x4 const mesh_xform = node->mTransformation * mtx;

                ice::mat4 ice_mtx;
                ice::memcpy(ice_mtx.v, &mesh_xform, sizeof(ice::mat4));

                model.mesh_list[mesh_idx].local_xform = ice::math::transpose(
                    ice_mtx
                );

                ice::detail::process_mesh(
                    model,
                    model.mesh_list[mesh_idx],
                    scene->mMeshes[mesh_idx],
                    node->mTransformation * mtx
                );
            }

            for (ice::u32 i = 0; i < node->mNumChildren; ++i)
            {
                ice::detail::process_node(
                    model,
                    node->mChildren[i],
                    scene,
                    node->mTransformation * mtx
                );
            }
        }

    } // namespace detail

    auto IceshardMeshOven::bake(
        ice::Data resource_data,
        ice::Metadata const& resource_meta,
        ice::ResourceSystem& resource_system,
        ice::Allocator& asset_alloc,
        ice::Memory& asset_data
    ) noexcept -> ice::BakeResult
    {
        Assimp::Importer importer;
        //importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 80);

        aiScene const* scene = nullptr;

        ice::String hint;
        if (ice::meta_read_string(resource_meta, "ice.asset.baking_hint"_sid, hint))
        {
            scene = importer.ReadFileFromMemory(
                resource_data.location,
                resource_data.size,
                aiProcessPreset_TargetRealtime_Fast,
                ice::string::data(hint)
            );
        }
        else
        {
            scene = importer.ReadFileFromMemory(
                resource_data.location,
                resource_data.size,
                aiProcessPreset_TargetRealtime_Fast
            );
        }

        // We don't know this file format
        if (scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || scene->mRootNode == nullptr)
        {
            return ice::BakeResult::Failure_InvalidData;
        }

        if (scene->mNumMeshes == 0)
        {
            return ice::BakeResult::Failure_InvalidData;
        }

        {
            ice::u32 vertice_offset = 0;
            ice::u32 indice_offset = 0;

            for (ice::u32 mesh_idx = 0; mesh_idx < scene->mNumMeshes; ++mesh_idx)
            {
                aiMesh const* const scene_mesh = scene->mMeshes[mesh_idx];
                vertice_offset += scene_mesh->mNumVertices;
                indice_offset += scene_mesh->mNumFaces * 3;
            }

            ice::u32 const vertice_data_size = vertice_offset * sizeof(ice::render::Vertice);
            ice::u32 const indice_data_size = indice_offset * sizeof(ice::u16);

            ice::u32 total_data_size = 0;
            total_data_size += sizeof(ice::render::Model);
            total_data_size += sizeof(ice::render::Mesh) * scene->mNumMeshes;
            total_data_size += vertice_data_size;
            total_data_size += indice_data_size;
            total_data_size += alignof(ice::math::vec3f) * 2 * scene->mNumMeshes;

            ice::Buffer out_data{ asset_alloc };
            ice::buffer::reserve(out_data, total_data_size);

            void* const model_ptr = ice::buffer::append(
                out_data,
                ice::Data{
                    .size = sizeof(ice::render::Model),
                    .alignment = alignof(ice::render::Model)
                }
            );

            void* const mesh_ptr = ice::buffer::append(
                out_data,
                ice::Data{
                    .size = sizeof(ice::render::Mesh) * scene->mNumMeshes,
                    .alignment = alignof(ice::render::Mesh)
                }
            );

            ice::render::Model* model = reinterpret_cast<ice::render::Model*>(model_ptr);
            model->mesh_count = scene->mNumMeshes;
            model->mesh_list = reinterpret_cast<ice::render::Mesh*>(mesh_ptr);
            model->vertice_data = nullptr;
            model->vertice_data_size = vertice_data_size;
            model->indice_data = nullptr;
            model->indice_data_size = indice_data_size;

            detail::MutableModel mutable_model;
            mutable_model.mesh_count = scene->mNumMeshes;
            mutable_model.mesh_list = reinterpret_cast<ice::render::Mesh*>(mesh_ptr);
            mutable_model.vertice_data = nullptr;
            mutable_model.vertice_data_size = vertice_data_size;
            mutable_model.indice_data = nullptr;
            mutable_model.indice_data_size = indice_data_size;

            vertice_offset = 0;
            indice_offset = 0;

            for (ice::u32 mesh_idx = 0; mesh_idx < model->mesh_count; ++mesh_idx)
            {
                aiMesh const* const scene_mesh = scene->mMeshes[mesh_idx];
                ice::render::Mesh& model_mesh = mutable_model.mesh_list[mesh_idx];

                model_mesh.vertice_count = scene_mesh->mNumVertices;
                model_mesh.vertice_offset = vertice_offset;
                vertice_offset += model_mesh.vertice_count;

                model_mesh.indice_count = scene_mesh->mNumFaces * 3;
                model_mesh.indice_offset = indice_offset;
                indice_offset += model_mesh.indice_count;
            }

            void* const vertice_ptr = ice::buffer::append(
                out_data,
                ice::Data{
                    .size = model->vertice_data_size,
                    .alignment = alignof(ice::render::Vertice)
                }
            );

            void* const indice_ptr = ice::buffer::append(
                out_data,
                ice::Data{
                    .size = model->indice_data_size,
                    .alignment = alignof(ice::u16)
                }
            );

            mutable_model.vertice_data = reinterpret_cast<ice::render::Vertice*>(vertice_ptr);
            mutable_model.indice_data = reinterpret_cast<ice::u16*>(indice_ptr);

            detail::process_node(
                mutable_model,
                scene->mRootNode,
                scene,
                aiMatrix4x4{ }
            );

            void* const data_begin = ice::buffer::data(out_data);

            model->mesh_list = reinterpret_cast<ice::render::Mesh*>(
                static_cast<ice::uptr>(
                    ice::memory::ptr_distance(data_begin, mesh_ptr)
                )
            );

            model->vertice_data = reinterpret_cast<ice::render::Vertice*>(
                static_cast<ice::uptr>(
                    ice::memory::ptr_distance(data_begin, vertice_ptr)
                )
            );

            model->indice_data = reinterpret_cast<ice::u16*>(
                static_cast<ice::uptr>(
                    ice::memory::ptr_distance(data_begin, indice_ptr)
                )
            );

            asset_data = ice::buffer::extrude_memory(out_data);
        }

        return ice::BakeResult::Success;
    }

} // namespace ice
