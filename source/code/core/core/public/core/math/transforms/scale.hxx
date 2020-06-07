#pragma once
#include <core/math/matrix.hxx>

namespace core::math
{

    constexpr auto scale(mat4x4 left, vec3f right) noexcept -> mat4x4
    {
        mat4x4 result{ };
        for (u32 col = 0; col < 3; ++col)
        {
            for (u32 row = 0; row < 4; ++row)
            {
                result.v[col][row] = left.v[col][row] * right.v[0][row];
            }
        }
        for (u32 row = 0; row < 4; ++row)
        {
            result.v[3][row] = left.v[3][row];
        }
        return result;
    }

    constexpr auto scale(vec3f v) noexcept -> mat4x4
    {
        return scale(identity<mat4x4>(), v);
    }

} // namespace core::math
