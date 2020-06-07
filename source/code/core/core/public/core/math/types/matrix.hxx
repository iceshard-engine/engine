#pragma once
#include <core/math/base.hxx>

namespace core::math
{

    template<u32 Rows, u32 Cols, typename T>
    struct mat
    {
        using value_type = T;
        static constexpr auto RowCount = Rows;
        static constexpr auto ColumnCount = Cols;

        T v[Cols][Rows];
    };

    using mat2x2 = mat<2, 2, f32>;
    using mat2 = mat2x2;

    using mat3x3 = mat<3, 3, f32>;
    using mat3 = mat3x3;

    using mat4x4 = mat<4, 4, f32>;
    using mat4 = mat4x4;


    template<typename Mat>
    constexpr auto identity() noexcept
    {
        static_assert(Mat::ColumnCount == Mat::RowCount, "Only even martices can be used with 'identity()'.");

        Mat result{ };
        for (u32 i = 0; i < Mat::ColumnCount; ++i)
        {
            result.v[i][i] = typename Mat::value_type{ 1 };
        }
        return result;
    }

    template<typename T>
    inline auto matrix_from_data(T const& data) noexcept -> mat4x4
    {
        static_assert(sizeof(mat4x4) == sizeof(T));

        mat4x4 result;
        memcpy(&result, &data, sizeof(T));
        return result;
    }

} // namespace core::math
