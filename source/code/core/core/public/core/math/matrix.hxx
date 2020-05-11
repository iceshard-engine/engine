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

    inline auto radians(f32 degrees) noexcept -> f32
    {
        static constexpr f32 rads_in_deg = core::math::pi / 180.f;
        return degrees * rads_in_deg;
    }

    inline auto sqrt(f32 val) noexcept -> f32
    {
        return std::sqrtf(val);
    }

    inline auto sin(f32 radians) noexcept -> f32
    {
        return std::sinf(radians);
    }

    inline auto cos(f32 radians) noexcept -> f32
    {
        return std::cosf(radians);
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

    inline auto normalize(vec3 val) noexcept -> vec3
    {
        f32 const invsqrt = 1.f / sqrt(val.x * val.x + val.y * val.y + val.z * val.z);
        return { val.x * invsqrt, val.y * invsqrt, val.z * invsqrt };
    }

    inline auto translate(mat4x4 left, vec3 right) noexcept -> mat4x4
    {
        left.v[3][0] += right.x;
        left.v[3][1] += right.y;
        left.v[3][2] += right.z;
        return left;
    }

    inline auto translate(vec3 displacement) noexcept -> mat4x4
    {
        return translate(identity<mat4x4>(), displacement);
    }

    inline auto scale(mat4x4 left, vec3 right) noexcept -> mat4x4
    {
        left.v[0][0] *= right.x;
        left.v[1][1] *= right.y;
        left.v[2][2] *= right.z;
        return left;
    }

    inline auto scale(vec3 v) noexcept -> mat4x4
    {
        return scale(identity<mat4x4>(), v);
    }

    inline auto rotate(mat4x4 left, f32 rad, vec3 v) noexcept -> mat4x4
    {
        f32 const angle = rad;
        f32 const cosv = cos(rad);
        f32 const cosv_1 = 1.f - cosv;
        f32 const sinv = sin(rad);

        vec3 const axis = normalize(v);
        vec3 const cos1_axis = { cosv_1 * axis.x, cosv_1 * axis.y, cosv_1 * axis.z };

        mat4x4 rm = identity<mat4x4>();
        rm.v[0][0] = cosv + cos1_axis.x * axis.x;
        rm.v[0][1] = cos1_axis.x * axis.y + sinv * axis.z;
        rm.v[0][2] = cos1_axis.x * axis.z - sinv * axis.y;

        rm.v[1][0] = cos1_axis.x * axis.y - sinv * axis.z;
        rm.v[1][1] = cosv + cos1_axis.y * axis.y;
        rm.v[1][2] = cos1_axis.y * axis.z + sinv * axis.x;

        rm.v[2][0] = cos1_axis.x * axis.z + sinv * axis.y;
        rm.v[2][1] = cos1_axis.y * axis.z - sinv * axis.x;
        rm.v[2][2] = cosv + cos1_axis.z * axis.z;
        return mul(left, rm);
    }

    inline auto rotate(f32 rad, vec3 v) noexcept -> mat4x4
    {
        return rotate(identity<mat4x4>(), rad, v);
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
