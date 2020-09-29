#pragma once
#include <core/math/matrix.hxx>
#include <core/math/vector.hxx>

namespace iceshard::renderer::v1
{

    struct Mesh
    {
        core::math::u32 vertice_count;
        core::math::u32 vertice_offset;
        core::math::u32 indice_count;
        core::math::u32 indice_offset;
        core::math::mat4x4 local_xform;
    };

    struct Vertice
    {
        core::math::vec3f pos;
        core::math::vec3f norm;
        core::math::vec2f uv;
    };

    struct Model
    {
        uint32_t mesh_count;
        uint32_t vertice_data_size;
        uint32_t indice_data_size;
        Mesh* mesh_list;
        Vertice* vertice_data;
        core::math::u16* indice_data;
    };

    struct ModelView
    {
        uint32_t mesh_count;
        uint32_t vertice_data_size;
        uint32_t indice_data_size;
        Mesh const* mesh_list;
        Vertice const* vertice_data;
        core::math::u16 const* indice_data;
    };


} // namespace iceshard
