/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/math.hxx>

namespace ice::math
{

    inline auto lookat(
        vec<3, f32> pos,
        vec<3, f32> target,
        vec<3, f32> up
    ) noexcept -> mat<4, 4, f32>;

    inline auto lookat(
        vec<3, f32> pos,
        vec<3, f32> target,
        vec<3, f32> up
    ) noexcept -> mat<4, 4, f32>
    {
        vec<3, f32> const dir = normalize(pos - target);
        vec<3, f32> const right = normalize(cross(up, dir));
        vec<3, f32> const view_up = cross(dir, right);

        mat<4, 4, f32> const temp {
            right.v[0][0], view_up.v[0][0], dir.v[0][0], 0.0f,
            right.v[0][1], view_up.v[0][1], dir.v[0][1], 0.0f,
            right.v[0][2], view_up.v[0][2], dir.v[0][2], 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
        };
        return temp * translate(-pos);
    }

} // namespace core::math
