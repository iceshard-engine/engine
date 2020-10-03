#pragma once
#include <ice/math/vector/vector_operations.hxx>

namespace ice::math
{

    template<u32 Size, typename T>
    constexpr auto operator-(vec<Size, T> left) noexcept -> vec<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto operator+(vec<Size, T> left, vec<Size, U> right) noexcept -> vec<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto operator-(vec<Size, T> left, vec<Size, U> right) noexcept -> vec<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto operator*(vec<Size, T> left, U right) noexcept -> vec<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto operator/(vec<Size, T> left, U right) noexcept -> vec<Size, T>;


    template<u32 Size, typename T>
    constexpr auto operator-(vec<Size, T> left) noexcept -> vec<Size, T>
    {
        vec<Size, T> result{ };
        for (u32 i = 0; i < Size; ++i)
        {
            result.v[0][i] = -left.v[0][i];
        }
        return result;
    }

    template<u32 Size, typename T, typename U = T>
    constexpr auto operator+(vec<Size, T> left, vec<Size, U> right) noexcept -> vec<Size, T>
    {
        return ice::math::add(left, right);
    }

    template<u32 Size, typename T, typename U = T>
    constexpr auto operator-(vec<Size, T> left, vec<Size, U> right) noexcept -> vec<Size, T>
    {
        return ice::math::sub(left, right);
    }

    template<u32 Size, typename T, typename U = T>
    constexpr auto operator*(vec<Size, T> left, U right) noexcept -> vec<Size, T>
    {
        return ice::math::mul(left, vec<Size, T>{ right });
    }

    template<u32 Size, typename T, typename U = T>
    constexpr auto operator/(vec<Size, T> left, U right) noexcept -> vec<Size, T>
    {
        return ice::math::div(left, vec<Size, T>{ right });
    }

} // namespace ice::math
