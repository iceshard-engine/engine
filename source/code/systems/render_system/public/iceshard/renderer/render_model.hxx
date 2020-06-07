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

    struct Model
    {
        uint32_t mesh_count;
        uint32_t vertice_data_size;
        uint32_t indice_data_size;
        Mesh* mesh_list;
        core::math::vec3f* vertice_data;
        core::math::u16* indice_data;
    };

} // namespace iceshard
