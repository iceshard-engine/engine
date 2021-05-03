#pragma once
#include <ice/math.hxx>

namespace ice::math
{

    constexpr auto orthographic(
        vec<2, f32> left_right,
        vec<2, f32> top_bottom,
        vec<2, f32> near_far
    ) noexcept -> mat<4, 4, f32>;

    constexpr auto orthographic(
        f32 left, f32 right,
        f32 bottom, f32 top,
        f32 nearv, f32 farv
    ) noexcept -> mat<4, 4, f32>;

    inline auto perspective(
        rad fovy,
        f32 aspect,
        f32 znear,
        f32 zfar
    ) noexcept -> mat<4, 4, f32>;


    constexpr auto orthographic(
        vec<2, f32> left_right,
        vec<2, f32> top_bottom,
        vec<2, f32> near_far
    ) noexcept -> mat<4, 4, f32>
    {
        return orthographic(
            left_right.v[0][0],
            left_right.v[0][1],
            top_bottom.v[0][1],
            top_bottom.v[0][0],
            near_far.v[0][0],
            near_far.v[0][1]
        );
    }

    constexpr auto orthographic(
        f32 left, f32 right,
        f32 bottom, f32 top,
        f32 nearv, f32 farv
    ) noexcept -> mat<4, 4, f32>
    {
        mat<4, 4, f32> result{ };
        // [issue #37] Pick a single coordinate system and check this code.
        //  As it might work for now but not be 100% valid.
        result.v[0][0] = 2.f / (right - left);
        result.v[1][1] = 2.f / (top - bottom);
        result.v[2][2] = 2.f / (farv - nearv);
        result.v[3][0] = -(right + left) / (right - left);
        result.v[3][1] = -(top + bottom) / (top - bottom);
        result.v[3][2] = -(farv + nearv) / (farv - nearv);
        result.v[3][3] = 1.f;
        return result;
    }

    inline auto perspective(
        rad fovy,
        f32 aspect,
        f32 znear,
        f32 zfar
    ) noexcept -> mat<4, 4, f32>
    {
        f32 const tan_half_fovy = tan(rad{ fovy.value / 2.0f });

        mat<4, 4, f32> result{ };
        result.v[0][0] = 1.0f / (aspect * tan_half_fovy);
        result.v[1][1] = 1.0f / tan_half_fovy;
        result.v[2][2] = -(zfar + znear) / (zfar - znear);
        result.v[2][3] = -1.0f;
        result.v[3][2] = -(2.0f * zfar * znear) / (zfar - znear);
        return result;
    }

} // namespace ice::math
