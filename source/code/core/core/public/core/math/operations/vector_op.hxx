#pragma once
#include <core/math/vector.hxx>

namespace core::math
{

    inline auto normalize(vec3f val) noexcept -> vec3f
    {
        f32 const invsqrt = 1.f / sqrt(val.x * val.x + val.y * val.y + val.z * val.z);
        return { val.x * invsqrt, val.y * invsqrt, val.z * invsqrt };
    }

    constexpr auto cross(vec3f left, vec3f right) noexcept -> vec3f
    {
        return {
            left.y * right.z - right.y * left.z,
            left.z * right.x - right.z * left.x,
            left.x * right.y - right.x * left.y
        };
    }

    template<u32 Size, typename T>
    constexpr auto dot(vec<Size, T> left, vec<Size, T> right) noexcept -> T
    {
        T result{ 0 };
        for (u32 i = 0; i < Size; ++i)
        {
            result += left.v[0][i] * right.v[0][i];
        }
        return result;
    }

    template<u32 Size, typename T, typename U = T>
    constexpr auto operator+(vec<Size, T> left, vec<Size, U> right) noexcept -> vec<Size, T>
    {
        for (u32 i = 0; i < Size; ++i)
        {
            left.v[0][i] += T{ right.v[0][i] };
        }
        return left;
    }

    template<u32 Size, typename T, typename U = T>
    inline auto operator+=(vec<Size, T>& left, vec<Size, U> right) noexcept -> vec<Size, T>&
    {
        for (u32 i = 0; i < Size; ++i)
        {
            left.v[0][i] += T{ right.v[0][i] };
        }
        return left;
    }

    template<u32 Size, typename T, typename U = T>
    constexpr auto operator-(vec<Size, T> left, vec<Size, U> right) noexcept -> vec<Size, T>
    {
        for (u32 i = 0; i < Size; ++i)
        {
            left.v[0][i] -= T{ right.v[0][i] };
        }
        return left;
    }

    template<u32 Size, typename T, typename U = T>
    inline auto operator-=(vec<Size, T>& left, vec<Size, U> right) noexcept -> vec<Size, T>&
    {
        for (u32 i = 0; i < Size; ++i)
        {
            left.v[0][i] -= T{ right.v[0][i] };
        }
        return left;
    }

    //template<u32 Size, typename T, typename U = T>
    //constexpr auto operator*(vec<Size, T> left, vec<Size, U> right) noexcept -> vec<Size, T>
    //{
    //    for (u32 i = 0; i < Size; ++i)
    //    {
    //        left.v[i] *= T{ right.v[i] };
    //    }
    //    return left;
    //}

    template<u32 Size, typename T, typename U = T>
    constexpr auto operator*(vec<Size, T> left, U scalar) noexcept -> vec<Size, T>
    {
        for (u32 i = 0; i < Size; ++i)
        {
            left.v[0][i] *= T{ scalar };
        }
        return left;
    }

    //template<u32 Size, typename T, typename U = T>
    //inline auto operator*=(vec<Size, T>& left, vec<Size, U> right) noexcept -> vec<Size, T>&
    //{
    //    for (u32 i = 0; i < Size; ++i)
    //    {
    //        left.v[i] *= T{ right.v[i] };
    //    }
    //    return left;
    //}

    template<u32 Size, typename T, typename U = T>
    inline auto operator*=(vec<Size, T>& left, U scalar) noexcept -> vec<Size, T>&
    {
        for (u32 i = 0; i < Size; ++i)
        {
            left.v[0][i] *= T{ scalar };
        }
        return left;
    }

    //template<u32 Size, typename T, typename U = T>
    //constexpr auto operator/(vec<Size, T> left, vec<Size, U> right) noexcept -> vec<Size, T>
    //{
    //    for (u32 i = 0; i < Size; ++i)
    //    {
    //        left.v[i] /= T{ right.v[i] };
    //    }
    //    return left;
    //}

    template<u32 Size, typename T, typename U = T>
    constexpr auto operator/(vec<Size, T> left, U scalar) noexcept -> vec<Size, T>
    {
        for (u32 i = 0; i < Size; ++i)
        {
            left.v[0][i] /= T{ scalar };
        }
        return left;
    }

    //template<u32 Size, typename T, typename U = T>
    //inline auto operator/=(vec<Size, T>& left, vec<Size, U> right) noexcept -> vec<Size, T>&
    //{
    //    for (u32 i = 0; i < Size; ++i)
    //    {
    //        left.v[i] /= T{ right.v[i] };
    //    }
    //    return left;
    //}

    template<u32 Size, typename T, typename U = T>
    inline auto operator/=(vec<Size, T>& left, U scalar) noexcept -> vec<Size, T>&
    {
        for (u32 i = 0; i < Size; ++i)
        {
            left.v[0][i] /= T{ scalar };
        }
        return left;
    }

} // namespace core::math
