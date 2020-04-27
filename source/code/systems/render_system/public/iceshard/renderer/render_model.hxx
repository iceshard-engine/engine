#pragma once
#include <core/base.hxx>

namespace core::math
{

    template<uint32_t Size, typename T>
    struct vec
    {
        T v[Size];
    };


    using f32 = float;
    using i16 = int16_t;
    using i32 = int32_t;
    using u16 = uint16_t;
    using u32 = uint32_t;

    using vec3 = vec<3, f32>;
    using uvec3 = vec<3, u32>;
    using vec4 = vec<4, f32>;
    using uvec4 = vec<4, u32>;

    struct mat4x4
    {
        vec4 r0;
        vec4 r1;
        vec4 r2;
        vec4 r3;
    };

}

namespace iceshard::renderer::v1
{

    struct Mesh
    {
        core::math::u32 vertice_count;
        core::math::u32 vertice_offset;
        core::math::u32 indice_count;
        core::math::u32 indice_offset;
    };

    struct Model
    {
        uint32_t mesh_count;
        uint32_t vertice_data_size;
        uint32_t indice_data_size;
        Mesh* mesh_list;
        core::math::vec3* vertice_data;
        core::math::u16* indice_data;
    };

} // namespace iceshard
