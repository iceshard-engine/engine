/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/math/common.hxx>
#include <ice/math/matrix/matrix_operations.hxx>
#include <ice/math/vector/vector_operations.hxx>

namespace ice::math
{

    inline auto rotate(rad rad, vec<3, f32> v) noexcept -> mat<4, 4, f32>;

    inline auto rotate(mat<4, 4, f32> left, rad rad, vec<3, f32> v) noexcept -> mat<4, 4, f32>;

    inline auto rotation(mat<4, 4, f32> const& matrix) noexcept -> vec<3, rad>;


    inline auto rotate(rad rad, vec<3, f32> v) noexcept -> mat<4, 4, f32>
    {
        return rotate(mat4x4_identity, rad, v);
    }

    inline auto rotate(mat<4, 4, f32> left, rad rad, vec<3, f32> v) noexcept -> mat<4, 4, f32>
    {
        f32 const cosv = cos(rad);
        f32 const cosv_1 = 1.f - cosv;
        f32 const sinv = sin(rad);

        vec<3, f32> const axis = normalize(v);
        vec<3, f32> const cos1_vec = {
            cosv_1 * axis.v[0][0],
            cosv_1 * axis.v[0][1],
            cosv_1 * axis.v[0][2]
        };

        mat<4, 4, f32> rm = mat4x4_identity;
        rm.v[0][0] = cosv + cos1_vec.v[0][0] * axis.v[0][0];
        rm.v[0][1] = cos1_vec.v[0][0] * axis.v[0][1] + sinv * axis.v[0][2];
        rm.v[0][2] = cos1_vec.v[0][0] * axis.v[0][2] - sinv * axis.v[0][1];

        rm.v[1][0] = cos1_vec.v[0][0] * axis.v[0][1] - sinv * axis.v[0][2];
        rm.v[1][1] = cosv + cos1_vec.v[0][1] * axis.v[0][1];
        rm.v[1][2] = cos1_vec.v[0][1] * axis.v[0][2] + sinv * axis.v[0][0];

        rm.v[2][0] = cos1_vec.v[0][0] * axis.v[0][2] + sinv * axis.v[0][1];
        rm.v[2][1] = cos1_vec.v[0][1] * axis.v[0][2] - sinv * axis.v[0][0];
        rm.v[2][2] = cosv + cos1_vec.v[0][2] * axis.v[0][2];
        return mul(left, rm);
    }

    inline auto rotation(mat<4, 4, f32> const& matrix) noexcept -> vec<3, rad>
    {
        rad const x = ice::math::atan2({
            matrix.v[1][2],
            matrix.v[2][2]
        });
        rad const y = ice::math::atan2({
            -matrix.v[0][2],
            ice::math::sqrt(matrix.v[1][2] * matrix.v[1][2] + matrix.v[2][2] * matrix.v[2][2])
        });
        rad const z = ice::math::atan2({
            matrix.v[0][1],
            matrix.v[0][0]
        });
        return { x, y, z };
    }

} // namespace ice::math
