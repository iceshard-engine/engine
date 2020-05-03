#pragma once
#include <core/math/vector.hxx>

namespace core::math
{

    template<uint32_t Rows, uint32_t Cols, typename T>
    struct mat
    {
        using ValueType = T;
        static constexpr auto RowCount = Rows;
        static constexpr auto ColumnCount = Cols;

        T v[Rows][Cols];
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
        static_assert(false, "Unknown matrix type, no 'identity' defined.");
    }

    template<>
    constexpr auto identity<mat2x2>() noexcept
    {
        mat2x2 result{ };
        result.v[0][0] = 1;
        result.v[1][1] = 1;
        return result;
    }

    template<>
    constexpr auto identity<mat3x3>() noexcept
    {
        mat3x3 result{ };
        result.v[0][0] = 1;
        result.v[1][1] = 1;
        result.v[2][2] = 1;
        return result;
    }

    template<>
    constexpr auto identity<mat4x4>() noexcept
    {
        mat4x4 result{ };
        result.v[0][0] = 1;
        result.v[1][1] = 1;
        result.v[2][2] = 1;
        result.v[3][3] = 1;
        return result;
    }

    inline auto mul(mat4x4 left, mat4x4 right) noexcept -> mat4x4
    {
        mat4x4 result;
        for (uint32_t col = 0; col < mat4x4::ColumnCount; ++col)
        {
            for (uint32_t row = 0; row < mat4x4::RowCount; ++row)
            {
                result.v[col][row]
                    = left.v[col][0] * right.v[0][row]
                    + left.v[col][1] * right.v[1][row]
                    + left.v[col][2] * right.v[2][row]
                    + left.v[col][3] * right.v[3][row];
            }
        }
        return result;
    }

    inline auto mul(vec4 left, mat4x4 right) noexcept -> vec4
    {
        return vec4{
            left.x * right.v[0][0] + left.y * right.v[0][1] + left.z * right.v[0][2] + 1.0f * right.v[0][3],
            left.x * right.v[1][0] + left.y * right.v[1][1] + left.z * right.v[1][2] + 1.0f * right.v[1][3],
            left.x * right.v[2][0] + left.y * right.v[2][1] + left.z * right.v[2][2] + 1.0f * right.v[2][3],
            left.x * right.v[3][0] + left.y * right.v[3][1] + left.z * right.v[3][2] + 1.0f * right.v[3][3],
        };
    }

    inline auto translate(mat4x4 left, vec3 right) noexcept -> mat4x4
    {
        left.v[3][0] += right.x;
        left.v[3][1] += right.y;
        left.v[3][2] += right.z;
        return left;
    }

    inline auto scale(mat4x4 left, vec3 right) noexcept -> mat4x4
    {
        left.v[0][0] *= right.x;
        left.v[1][1] *= right.y;
        left.v[2][2] *= right.z;
        return left;
    }

    inline auto transpose(mat4x4 origin) noexcept -> mat4x4
    {
        return mat4x4{
            origin.v[0][0], origin.v[1][0], origin.v[2][0], origin.v[3][0],
            origin.v[0][1], origin.v[1][1], origin.v[2][1], origin.v[3][1],
            origin.v[0][2], origin.v[1][2], origin.v[2][2], origin.v[3][2],
            origin.v[0][3], origin.v[1][3], origin.v[2][3], origin.v[3][3],
        };
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
