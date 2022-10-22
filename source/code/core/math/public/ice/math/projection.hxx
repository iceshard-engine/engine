/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/math.hxx>

namespace ice::math
{

    inline auto perspective_fovx(
        rad field_of_view_horizontal,
        f32 aspect_ratio,
        f32 near_plane,
        f32 far_plane
    ) noexcept -> mat<4, 4, f32>;

    inline auto perspective_fovy(
        rad field_of_view_vertical,
        f32 aspect_ratio,
        f32 near_plane,
        f32 far_plane
    ) noexcept -> mat<4, 4, f32>;

    constexpr auto orthographic(
        f32 left, f32 right,
        f32 bottom, f32 top,
        f32 near_plane, f32 far_plane
    ) noexcept -> mat<4, 4, f32>;

    namespace math_detail
    {

        constexpr auto perspective_rhs_lrtb(
            f32 left,
            f32 right,
            f32 bottom,
            f32 top,
            f32 near_plane,
            f32 far_plane
        ) noexcept -> mat<4, 4, f32>
        {
            mat<4, 4, f32> result{ };

            result.v[0][0] = 2.f * near_plane / (right - left);
            result.v[1][1] = 2.f * near_plane / (top - bottom);
            result.v[2][0] = (right + left) / (right - left);
            result.v[2][1] = (top + bottom) / (top - bottom);
            result.v[2][2] = far_plane / (near_plane - far_plane);
            result.v[2][3] = -1.f;
            result.v[3][2] = (near_plane * far_plane) / (near_plane - far_plane);

            // #NOTE: The below values should be used when we want to produce clip.z between -1.f .. 1.f
            //result.v[2][2] = (plane_far + plane_near) / (plane_near - plane_far);
            //result.v[2][3] = -1.f;
            //result.v[3][2] = 2 * (plane_far * plane_near) / (plane_near - plane_far);
            return result;
        }

    } // namespace detail

    inline auto perspective_fovx(
        rad field_of_view_horizontal,
        f32 aspect_ratio,
        f32 near_plane,
        f32 far_plane
    ) noexcept -> mat<4, 4, f32>
    {
        f32 const tan_half_fovx = ice::math::tan(rad{ field_of_view_horizontal.value * 0.5f });

        f32 right = tan_half_fovx * near_plane;
        f32 left = -right;
        f32 top = right / aspect_ratio;
        f32 bottom = -top;

        return math_detail::perspective_rhs_lrtb(left, right, bottom, top, near_plane, far_plane);
    }

    inline auto perspective_fovy(
        rad field_of_view_vertical,
        f32 aspect_ratio,
        f32 near_plane,
        f32 far_plane
    ) noexcept -> mat<4, 4, f32>
    {
        f32 const tan_half_fovy = ice::math::tan(rad{ field_of_view_vertical.value * 0.5f });

        f32 top = tan_half_fovy * near_plane;
        f32 bottom = -top;
        f32 right = top * aspect_ratio;
        f32 left = -right;

        return math_detail::perspective_rhs_lrtb(left, right, bottom, top, near_plane, far_plane);
    }

    constexpr auto orthographic(
        vec<2, f32> left_right,
        vec<2, f32> bottom_top,
        vec<2, f32> near_far
    ) noexcept -> mat<4, 4, f32>
    {
        return orthographic(
            left_right.v[0][0],
            left_right.v[0][1],
            bottom_top.v[0][0],
            bottom_top.v[0][1],
            near_far.v[0][0],
            near_far.v[0][1]
        );
    }

    constexpr auto orthographic(
        f32 left, f32 right,
        f32 bottom, f32 top,
        f32 near_plane, f32 far_plane
    ) noexcept -> mat<4, 4, f32>
    {
        mat<4, 4, f32> result{ };

        result.v[0][0] = 2.f / (right - left);
        result.v[1][1] = 2.f / (top - bottom);
        result.v[2][2] = 1.f / (near_plane - far_plane);
        result.v[3][0] = -(right + left) / (right - left);
        result.v[3][1] = -(top + bottom) / (top - bottom);
        result.v[3][2] = near_plane / (near_plane - far_plane);
        result.v[3][3] = 1.f;
        return result;
    }

} // namespace ice::math
