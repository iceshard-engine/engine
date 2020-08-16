#pragma once
#include <core/math/types/matrix.hxx>

namespace core::math
{

    //
    // Multiplication
    template<u32 Rows1, u32 Cols1, u32 Rows2, u32 Cols2, typename T, typename U = T>
    constexpr auto mul(mat<Rows1, Cols1, T> left, mat<Rows2, Cols2, U> right) noexcept -> mat<Rows1, Cols2, T>
    {
        static_assert(Cols1 == Rows2, "[ left_mtx.col_count != right_mtx.row_count ] Cannot multiply given matrices.");
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

    //
    // Transposition
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

    //
    // Operators
    template<u32 Rows1, u32 Cols1, u32 Rows2, u32 Cols2, typename T, typename U = T>
    constexpr auto operator*(mat<Rows1, Cols1, T> left, mat<Rows2, Cols2, U> right) noexcept -> mat<Rows1, Cols2, T>
    {
        return mul(left, right);
    }

} // namespace core::math