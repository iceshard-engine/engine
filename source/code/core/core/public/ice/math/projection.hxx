#pragma once
#include <ice/math.hxx>

namespace ice::math
{

    inline auto perspective_fovx(
        rad field_of_view_horizontal,
        f32 aspect_ratio,
        f32 plane_near,
        f32 plane_far
    ) noexcept -> mat<4, 4, f32>;

    inline auto perspective_fovy(
        rad field_of_view_vertical,
        f32 aspect_ratio,
        f32 plane_near,
        f32 plane_far
    ) noexcept -> mat<4, 4, f32>;

    constexpr auto orthographic(
        f32 left, f32 right,
        f32 bottom, f32 top,
        f32 nearv, f32 farv
    ) noexcept -> mat<4, 4, f32>;


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

    namespace math_detail
    {

        constexpr auto perspective_rhs_lrtb(
            f32 left,
            f32 right,
            f32 bottom,
            f32 top,
            f32 plane_near,
            f32 plane_far
        ) noexcept -> mat<4, 4, f32>
        {
            mat<4, 4, f32> result{ };
            result.v[0][0] = 2.f * plane_near / (right - left);

            // #todo: Remove the negation as it's only valid for the Vulkan renderer. #54
            //  This should be fixed when proper works starts on additional renderer implementations.
            result.v[1][1] = -1.f * 2.f * plane_near / (top - bottom);
            result.v[2][0] = (right + left) / (right - left);
            result.v[2][1] = (top + bottom) / (top - bottom);
            result.v[2][2] = plane_far / (plane_near - plane_far);
            result.v[2][3] = -1.f;
            result.v[3][2] = (plane_near * plane_far) / (plane_near - plane_far);

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
        f32 plane_near,
        f32 plane_far
    ) noexcept -> mat<4, 4, f32>
    {
        f32 const tan_half_fovx = ice::math::tan(rad{ field_of_view_horizontal.value * 0.5f });

        f32 right = tan_half_fovx * plane_near;
        f32 left = -right;
        f32 top = right / aspect_ratio;
        f32 bottom = -top;

        return math_detail::perspective_rhs_lrtb(left, right, bottom, top, plane_near, plane_far);
    }

    inline auto perspective_fovy(
        rad field_of_view_vertical,
        f32 aspect_ratio,
        f32 plane_near,
        f32 plane_far
    ) noexcept -> mat<4, 4, f32>
    {
        f32 const tan_half_fovy = ice::math::tan(rad{ field_of_view_vertical.value * 0.5f });

        f32 top = tan_half_fovy * plane_near;
        f32 bottom = -top;
        f32 right = top * aspect_ratio;
        f32 left = -right;

        return math_detail::perspective_rhs_lrtb(left, right, bottom, top, plane_near, plane_far);
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

} // namespace ice::math
