#pragma once
#include <core/math/matrix.hxx>
#include <core/math/functions.hxx>

namespace core::math
{

    inline auto rotate(mat4x4 left, f32 rad, vec3f v) noexcept -> mat4x4
    {
        f32 const angle = rad;
        f32 const cosv = cos(rad);
        f32 const cosv_1 = 1.f - cosv;
        f32 const sinv = sin(rad);

        vec3f const axis = normalize(v);
        vec3f const cos1_axis = { cosv_1 * axis.x, cosv_1 * axis.y, cosv_1 * axis.z };

        mat4x4 rm = identity<mat4x4>();
        rm.v[0][0] = cosv + cos1_axis.x * axis.x;
        rm.v[0][1] = cos1_axis.x * axis.y + sinv * axis.z;
        rm.v[0][2] = cos1_axis.x * axis.z - sinv * axis.y;

        rm.v[1][0] = cos1_axis.x * axis.y - sinv * axis.z;
        rm.v[1][1] = cosv + cos1_axis.y * axis.y;
        rm.v[1][2] = cos1_axis.y * axis.z + sinv * axis.x;

        rm.v[2][0] = cos1_axis.x * axis.z + sinv * axis.y;
        rm.v[2][1] = cos1_axis.y * axis.z - sinv * axis.x;
        rm.v[2][2] = cosv + cos1_axis.z * axis.z;
        return mul(left, rm);
    }

    inline auto rotate(f32 rad, vec3f v) noexcept -> mat4x4
    {
        return rotate(identity<mat4x4>(), rad, v);
    }

} // namespace core::math
