#pragma once
#include "translate.hxx"

namespace core::math
{

    inline auto perspective(f32 fovy, f32 aspect, f32 znear, f32 zfar) noexcept -> mat4x4
    {
        IS_ASSERT(
            abs(aspect - std::numeric_limits<f32>::epsilon()) > 0.0f,
            "Aspect ration is lower than minimal 'epsilon' value!"
        );

        f32 const tan_half_fovy = tan(fovy / 2.0f);

        mat4 result{ };
        result.v[0][0] = 1.0f / (aspect * tan_half_fovy);
        result.v[1][1] = 1.0f / tan_half_fovy;
        result.v[2][2] = -(zfar + znear) / (zfar - znear);
        result.v[2][3] = -1.0f;
        result.v[3][2] = -(2.0f * zfar * znear) / (zfar - znear);
        return result;
    }

} // namespace core::math
