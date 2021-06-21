#pragma once
#include <ice/math/types.hxx>

namespace ice::math
{

    template<u32 Rows, u32 Cols, typename T>
    struct mat
    {
        using value_type = T;
        static constexpr auto count_rows = Rows;
        static constexpr auto count_columns = Cols;

        T v[count_columns][count_rows];
    };


    template<typename Mat>
    constexpr auto identity() noexcept;


    using mat2x2 = mat<2, 2, f32>;
    using mat2 = mat2x2;

    using mat3x3 = mat<3, 3, f32>;
    using mat3 = mat3x3;

    using mat4x4 = mat<4, 4, f32>;
    using mat4 = mat4x4;


    template<typename Mat>
    constexpr auto identity() noexcept
    {
        static_assert(
            Mat::count_columns == Mat::count_rows,
            "Only even martices can be used with 'core::math::identity()'"
            );

        Mat result{ };
        for (u32 col = 0; col < Mat::count_columns; ++col)
        {
            result.v[col][col] = typename Mat::value_type{ 1 };
        }
        return result;
    }


    static constexpr auto mat2x2_identity = identity<mat<2, 2, f32>>();
    static constexpr auto mat3x3_identity = identity<mat<3, 3, f32>>();
    static constexpr auto mat4x4_identity = identity<mat<4, 4, f32>>();

} // namespace ice::math
