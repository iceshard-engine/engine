#pragma once
#include <ice/math/vector.hxx>
#include <ice/math/matrix/matrix_operations.hxx>
#include <ice/math/matrix/matrix_operators.hxx>

namespace ice::math
{

    constexpr auto scale(vec<3, f32> v) noexcept -> mat<4, 4, f32>;

    constexpr auto scale(mat<4, 4, f32> left, vec<3, f32> right) noexcept -> mat<4, 4, f32>;


    constexpr auto scale(vec<3, f32> v) noexcept -> mat<4, 4, f32>
    {
        return scale(mat4x4_identity, v);
    }

    constexpr auto scale(mat<4, 4, f32> left, vec<3, f32> right) noexcept -> mat<4, 4, f32>
    {
        mat<4, 4, f32> temp{ };
        temp.v[0][0] = right.v[0][0];
        temp.v[1][1] = right.v[0][1];
        temp.v[2][2] = right.v[0][2];
        temp.v[3][3] = 1.f;
        return mul(left, temp);
    }

} // namespace ice::math
