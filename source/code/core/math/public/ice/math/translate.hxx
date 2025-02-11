/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/math/vector.hxx>

namespace ice::math
{

    constexpr auto translate(vec<3, f32> displacement) noexcept -> mat<4, 4, f32>;

    constexpr auto translate(mat<4, 4, f32> left, vec<3, f32> right) noexcept -> mat<4, 4, f32>;

    constexpr auto translation(mat<4, 4, f32> const& matrix) noexcept -> vec<3, f32>;


    constexpr auto translate(vec<3, f32> displacement) noexcept -> mat<4, 4, f32>
    {
        return translate(mat4x4_identity, displacement);
    }

    constexpr auto translate(mat<4, 4, f32> left, vec<3, f32> right) noexcept -> mat<4, 4, f32>
    {
        left.v[3][0] += right.v[0][0];
        left.v[3][1] += right.v[0][1];
        left.v[3][2] += right.v[0][2];
        return left;
    }

    constexpr auto translation(mat<4, 4, f32> const& matrix) noexcept -> vec<3, f32>
    {
        return { matrix.v[3][0], matrix.v[3][1], matrix.v[3][2] };
    }

} // namespace ice::math
