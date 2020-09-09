#pragma once
#include "translate.hxx"

namespace core::math
{

    inline auto perspective(rad fovy, f32 aspect, f32 znear, f32 zfar) noexcept -> mat4x4
    {
        IS_ASSERT(
            abs(aspect - std::numeric_limits<f32>::epsilon()) > 0.0f,
            "Aspect ration is lower than minimal 'epsilon' value!"
        );

        f32 const tan_half_fovy = tan(rad{ fovy.value / 2.0f });

        mat4 result{ };
        result.v[0][0] = 1.0f / (aspect * tan_half_fovy);
        result.v[1][1] = 1.0f / tan_half_fovy;
        result.v[2][2] = -(zfar + znear) / (zfar - znear);
        result.v[2][3] = -1.0f;
        result.v[3][2] = -(2.0f * zfar * znear) / (zfar - znear);
        return result;
    }

    inline auto orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 nearv, f32 farv) noexcept -> mat4x4
    {
        mat4 result{ };
        result.v[0][0] = 2.f / (right - left);
        result.v[1][1] = 2.f / (bottom - top);
        result.v[2][2] = 1.f / (nearv - farv);
        result.v[3][0] = -(right + left) / (right - left);
        result.v[3][1] = -(bottom + top) / (bottom - top);
        result.v[3][2] = nearv / (nearv - farv);
        result.v[3][3] = 1.f;
        return result;
    }

} // namespace core::math
