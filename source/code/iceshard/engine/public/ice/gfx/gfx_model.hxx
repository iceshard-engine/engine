#pragma once
#include <ice/math.hxx>

namespace ice::gfx
{

    struct Vertice
    {
        ice::math::vec3f pos;
        ice::math::vec3f norm;
        ice::math::vec2f uv;
    };

    struct Mesh
    {
        ice::u32 vertice_count;
        ice::u32 vertice_offset;
        ice::u32 indice_count;
        ice::u32 indice_offset;
        ice::math::mat4x4 local_xform;
    };

    struct Model
    {
        ice::u32 mesh_count;
        ice::u32 vertice_data_size;
        ice::u32 indice_data_size;
        ice::u16 const* indice_data;
        ice::gfx::Vertice const* vertice_data;
        ice::gfx::Mesh const* mesh_list;
    };

} // namespace ice::gfx
