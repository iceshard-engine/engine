#pragma once
#include <ice/math/vector/vector_operations.hxx>
#include <ice/math/rotate.hxx>
#include <ice/math/scale.hxx>
#include <ice/math/translate.hxx>

namespace ice::math
{

    template<typename T>
    constexpr void decompose(
        mat<4, 4, T> matrix,
        vec3f* pos,
        vec3f* scale,
        vec<3, rad>* angle
    ) noexcept;


    template<typename T>
    constexpr void decompose(
        mat<4, 4, T> matrix,
        vec3f* pos,
        vec3f* scale,
        vec<3, rad>* angle
    ) noexcept
    {
        if (scale != nullptr)
        {
            *scale = ice::math::scale(matrix);
        }

        matrix = ice::math::ortho_normalize(matrix);

        if (pos != nullptr)
        {
            *pos = ice::math::translation(matrix);
        }
        if (angle != nullptr)
        {
            *angle = ice::math::rotation(matrix);
        }
    }

} // namespace ice::math
