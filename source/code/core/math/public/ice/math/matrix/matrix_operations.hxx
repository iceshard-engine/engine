/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/math/matrix.hxx>
#include <ice/math/vector/vector_operations.hxx>

namespace ice::math_detail
{

    template<typename T>
    constexpr void normalize3_insitu(T(&array)[4]) noexcept
    {
        T const square_sum = length2_sequenced(array, std::make_index_sequence<3>{});
        if (square_sum == 0)
        {
            return;
        }

        f32 const sqrt_inverted = 1.f / sqrt(square_sum);
        array[0] *= sqrt_inverted;
        array[1] *= sqrt_inverted;
        array[2] *= sqrt_inverted;
    }

} // namespace detail

namespace ice::math
{

    template<u32 Rows1, u32 Cols1, u32 Rows2, u32 Cols2, typename T, typename U = T>
    constexpr auto mul(
        mat<Rows1, Cols1, T> left,
        mat<Rows2, Cols2, U> right
    ) noexcept -> mat<Rows1, Cols2, T>;

    template<u32 Rows, u32 Cols, typename T>
    constexpr auto transpose(mat<Rows, Cols, T> matrix) noexcept -> mat<Cols, Rows, T>;

    template<u32 Rows, u32 Cols, typename T>
    constexpr bool inverse_insitu(mat<Rows, Cols, T>& m) noexcept
    {
        T inv[Rows * Cols], det;

        inv[0] = m.v[0][5] * m.v[0][10] * m.v[0][15] -
            m.v[0][5] * m.v[0][11] * m.v[0][14] -
            m.v[0][9] * m.v[0][6] * m.v[0][15] +
            m.v[0][9] * m.v[0][7] * m.v[0][14] +
            m.v[0][13] * m.v[0][6] * m.v[0][11] -
            m.v[0][13] * m.v[0][7] * m.v[0][10];

        inv[4] = -m.v[0][4] * m.v[0][10] * m.v[0][15] +
            m.v[0][4] * m.v[0][11] * m.v[0][14] +
            m.v[0][8] * m.v[0][6] * m.v[0][15] -
            m.v[0][8] * m.v[0][7] * m.v[0][14] -
            m.v[0][12] * m.v[0][6] * m.v[0][11] +
            m.v[0][12] * m.v[0][7] * m.v[0][10];

        inv[8] = m.v[0][4] * m.v[0][9] * m.v[0][15] -
            m.v[0][4] * m.v[0][11] * m.v[0][13] -
            m.v[0][8] * m.v[0][5] * m.v[0][15] +
            m.v[0][8] * m.v[0][7] * m.v[0][13] +
            m.v[0][12] * m.v[0][5] * m.v[0][11] -
            m.v[0][12] * m.v[0][7] * m.v[0][9];

        inv[12] = -m.v[0][4] * m.v[0][9] * m.v[0][14] +
            m.v[0][4] * m.v[0][10] * m.v[0][13] +
            m.v[0][8] * m.v[0][5] * m.v[0][14] -
            m.v[0][8] * m.v[0][6] * m.v[0][13] -
            m.v[0][12] * m.v[0][5] * m.v[0][10] +
            m.v[0][12] * m.v[0][6] * m.v[0][9];

        inv[1] = -m.v[0][1] * m.v[0][10] * m.v[0][15] +
            m.v[0][1] * m.v[0][11] * m.v[0][14] +
            m.v[0][9] * m.v[0][2] * m.v[0][15] -
            m.v[0][9] * m.v[0][3] * m.v[0][14] -
            m.v[0][13] * m.v[0][2] * m.v[0][11] +
            m.v[0][13] * m.v[0][3] * m.v[0][10];

        inv[5] = m.v[0][0] * m.v[0][10] * m.v[0][15] -
            m.v[0][0] * m.v[0][11] * m.v[0][14] -
            m.v[0][8] * m.v[0][2] * m.v[0][15] +
            m.v[0][8] * m.v[0][3] * m.v[0][14] +
            m.v[0][12] * m.v[0][2] * m.v[0][11] -
            m.v[0][12] * m.v[0][3] * m.v[0][10];

        inv[9] = -m.v[0][0] * m.v[0][9] * m.v[0][15] +
            m.v[0][0] * m.v[0][11] * m.v[0][13] +
            m.v[0][8] * m.v[0][1] * m.v[0][15] -
            m.v[0][8] * m.v[0][3] * m.v[0][13] -
            m.v[0][12] * m.v[0][1] * m.v[0][11] +
            m.v[0][12] * m.v[0][3] * m.v[0][9];

        inv[13] = m.v[0][0] * m.v[0][9] * m.v[0][14] -
            m.v[0][0] * m.v[0][10] * m.v[0][13] -
            m.v[0][8] * m.v[0][1] * m.v[0][14] +
            m.v[0][8] * m.v[0][2] * m.v[0][13] +
            m.v[0][12] * m.v[0][1] * m.v[0][10] -
            m.v[0][12] * m.v[0][2] * m.v[0][9];

        inv[2] = m.v[0][1] * m.v[0][6] * m.v[0][15] -
            m.v[0][1] * m.v[0][7] * m.v[0][14] -
            m.v[0][5] * m.v[0][2] * m.v[0][15] +
            m.v[0][5] * m.v[0][3] * m.v[0][14] +
            m.v[0][13] * m.v[0][2] * m.v[0][7] -
            m.v[0][13] * m.v[0][3] * m.v[0][6];

        inv[6] = -m.v[0][0] * m.v[0][6] * m.v[0][15] +
            m.v[0][0] * m.v[0][7] * m.v[0][14] +
            m.v[0][4] * m.v[0][2] * m.v[0][15] -
            m.v[0][4] * m.v[0][3] * m.v[0][14] -
            m.v[0][12] * m.v[0][2] * m.v[0][7] +
            m.v[0][12] * m.v[0][3] * m.v[0][6];

        inv[10] = m.v[0][0] * m.v[0][5] * m.v[0][15] -
            m.v[0][0] * m.v[0][7] * m.v[0][13] -
            m.v[0][4] * m.v[0][1] * m.v[0][15] +
            m.v[0][4] * m.v[0][3] * m.v[0][13] +
            m.v[0][12] * m.v[0][1] * m.v[0][7] -
            m.v[0][12] * m.v[0][3] * m.v[0][5];

        inv[14] = -m.v[0][0] * m.v[0][5] * m.v[0][14] +
            m.v[0][0] * m.v[0][6] * m.v[0][13] +
            m.v[0][4] * m.v[0][1] * m.v[0][14] -
            m.v[0][4] * m.v[0][2] * m.v[0][13] -
            m.v[0][12] * m.v[0][1] * m.v[0][6] +
            m.v[0][12] * m.v[0][2] * m.v[0][5];

        inv[3] = -m.v[0][1] * m.v[0][6] * m.v[0][11] +
            m.v[0][1] * m.v[0][7] * m.v[0][10] +
            m.v[0][5] * m.v[0][2] * m.v[0][11] -
            m.v[0][5] * m.v[0][3] * m.v[0][10] -
            m.v[0][9] * m.v[0][2] * m.v[0][7] +
            m.v[0][9] * m.v[0][3] * m.v[0][6];

        inv[7] = m.v[0][0] * m.v[0][6] * m.v[0][11] -
            m.v[0][0] * m.v[0][7] * m.v[0][10] -
            m.v[0][4] * m.v[0][2] * m.v[0][11] +
            m.v[0][4] * m.v[0][3] * m.v[0][10] +
            m.v[0][8] * m.v[0][2] * m.v[0][7] -
            m.v[0][8] * m.v[0][3] * m.v[0][6];

        inv[11] = -m.v[0][0] * m.v[0][5] * m.v[0][11] +
            m.v[0][0] * m.v[0][7] * m.v[0][9] +
            m.v[0][4] * m.v[0][1] * m.v[0][11] -
            m.v[0][4] * m.v[0][3] * m.v[0][9] -
            m.v[0][8] * m.v[0][1] * m.v[0][7] +
            m.v[0][8] * m.v[0][3] * m.v[0][5];

        inv[15] = m.v[0][0] * m.v[0][5] * m.v[0][10] -
            m.v[0][0] * m.v[0][6] * m.v[0][9] -
            m.v[0][4] * m.v[0][1] * m.v[0][10] +
            m.v[0][4] * m.v[0][2] * m.v[0][9] +
            m.v[0][8] * m.v[0][1] * m.v[0][6] -
            m.v[0][8] * m.v[0][2] * m.v[0][5];

        det = m.v[0][0] * inv[0] + m.v[0][1] * inv[4] + m.v[0][2] * inv[8] + m.v[0][3] * inv[12];
        if (det == 0)
        {
            return false;
        }

        det = 1.0f / det;
        for (u32 i = 0; i < 16; i++)
        {
            m.v[i >> 2][i & 0x3] = inv[i] * det;
        }

        return true;
    }

    template<typename T>
    constexpr auto ortho_normalize(mat<4, 4, T> matrix) noexcept -> mat<4, 4, T>;


    template<u32 Rows1, u32 Cols1, u32 Rows2, u32 Cols2, typename T, typename U>
    constexpr auto mul(
        mat<Rows1, Cols1, T> left,
        mat<Rows2, Cols2, U> right
    ) noexcept -> mat<Rows1, Cols2, T>
    {
        static_assert(
            Cols1 == Rows2,
            "[ left_matrix.count_columns != right_matrix.count_rows ] Cannot multiply given matrices."
        );

        if constexpr (Cols1 == Rows2)
        {
            mat<Rows1, Cols2, T> result{};
            for (u32 col = 0; col < Cols2; ++col)
            {
                for (u32 row = 0; row < Rows1; ++row)
                {
                    for (u32 i = 0; i < Rows2; ++i)
                    {
                        result.v[col][row] += left.v[i][row] * right.v[col][i];
                    }
                }

            }
            return result;
        }
        else
        {
            return { };
        }
    }

    template<u32 Rows, u32 Cols, typename T>
    constexpr auto transpose(mat<Rows, Cols, T> matrix) noexcept -> mat<Cols, Rows, T>
    {
        mat<Cols, Rows, T> result{ };
        for (u32 col = 0; col < Cols; ++col)
        {
            for (u32 row = 0; row < Rows; ++row)
            {
                result.v[row][col] = matrix.v[col][row];
            }
        }
        return result;
    }

    template<typename T>
    constexpr auto ortho_normalize(mat<4, 4, T> matrix) noexcept -> mat<4, 4, T>
    {
        ice::math_detail::normalize3_insitu(matrix.v[0]);
        ice::math_detail::normalize3_insitu(matrix.v[1]);
        ice::math_detail::normalize3_insitu(matrix.v[2]);
        return matrix;
    }

} // namespace ice::math
