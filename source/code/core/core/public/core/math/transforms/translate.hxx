#pragma once
#include <core/math/matrix.hxx>
#include <core/math/operations.hxx>

namespace core::math
{

    constexpr auto translate(mat4x4 left, vec3f right) noexcept -> mat4x4
    {
        left.v[3][0] += right.v[0][0];
        left.v[3][1] += right.v[0][1];
        left.v[3][2] += right.v[0][2];
        return left;
    }

    constexpr auto translate(vec3f displacement) noexcept -> mat4x4
    {
        return translate(identity<mat4x4>(), displacement);
    }

} // namespace core::math
