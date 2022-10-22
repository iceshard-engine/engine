/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/math/matrix/matrix_operations.hxx>

namespace ice::math
{

    template<u32 Rows1, u32 Cols1, u32 Rows2, u32 Cols2, typename T, typename U = T>
    constexpr auto operator*(
        mat<Rows1, Cols1, T> left,
        mat<Rows2, Cols2, U> right
    ) noexcept -> mat<Rows1, Cols2, T>
    {
        return ice::math::mul(left, right);
    }

} // namespace ice::math
