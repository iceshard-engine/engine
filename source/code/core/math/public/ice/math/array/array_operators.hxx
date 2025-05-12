/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/math/array/array_operations.hxx>

namespace ice::math
{

    template<u32 Size, typename T>
    constexpr auto operator-(arr_t<Size, T> left) noexcept -> arr_t<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto operator+(arr_t<Size, T> left, arr_t<Size, U> right) noexcept -> arr_t<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto operator+(vec<Size, T> left, arr_t<Size, U> right) noexcept -> vec<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto operator-(arr_t<Size, T> left, arr_t<Size, U> right) noexcept -> arr_t<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto operator-(vec<Size, T> left, arr_t<Size, U> right) noexcept -> vec<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto operator*(arr_t<Size, T> left, U right) noexcept -> arr_t<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto operator*(arr_t<Size, T> left, arr_t<Size, U> right) noexcept -> arr_t<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto operator*(vec<Size, T> left, arr_t<Size, U> right) noexcept -> vec<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto operator/(arr_t<Size, T> left, U right) noexcept -> arr_t<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto operator/(arr_t<Size, T> left, arr_t<Size, U> right) noexcept -> arr_t<Size, T>;

    template<u32 Size, typename T, typename U = T>
    constexpr auto operator/(vec<Size, T> left, arr_t<Size, U> right) noexcept -> vec<Size, T>;


    template<u32 Size, typename T>
    constexpr auto operator-(arr_t<Size, T> left) noexcept -> arr_t<Size, T>
    {
        arr_t<Size, T> result{ };
        for (u32 i = 0; i < Size; ++i)
        {
            result.v[i] = -left.v[i];
        }
        return result;
    }

    template<u32 Size, typename T, typename U>
    constexpr auto operator+(arr_t<Size, T> left, arr_t<Size, U> right) noexcept -> arr_t<Size, T>
    {
        return ice::math::add(left, right);
    }

    template<u32 Size, typename T, typename U>
    constexpr auto operator+(vec<Size, T> left, arr_t<Size, U> right) noexcept -> vec<Size, T>
    {
        return ice::math::add(left, right);
    }

    template<u32 Size, typename T, typename U>
    constexpr auto operator-(arr_t<Size, T> left, arr_t<Size, U> right) noexcept -> arr_t<Size, T>
    {
        return ice::math::sub(left, right);
    }

    template<u32 Size, typename T, typename U>
    constexpr auto operator-(vec<Size, T> left, arr_t<Size, U> right) noexcept -> vec<Size, T>
    {
        return ice::math::sub(left, right);
    }

    template<u32 Size, typename T, typename U>
    constexpr auto operator*(arr_t<Size, T> left, U right) noexcept -> arr_t<Size, T>
    {
        return ice::math::mul(left, right);
    }

    template<u32 Size, typename T, typename U>
    constexpr auto operator*(arr_t<Size, T> left, arr_t<Size, U> right) noexcept -> arr_t<Size, T>
    {
        return ice::math::mul(left, right);
    }

    template<u32 Size, typename T, typename U>
    constexpr auto operator*(vec<Size, T> left, arr_t<Size, U> right) noexcept -> vec<Size, T>
    {
        return ice::math::mul(left, right);
    }

    template<u32 Size, typename T, typename U>
    constexpr auto operator/(arr_t<Size, T> left, U right) noexcept -> arr_t<Size, T>
    {
        return ice::math::div(left, right);
    }

    template<u32 Size, typename T, typename U>
    constexpr auto operator/(arr_t<Size, T> left, arr_t<Size, U> right) noexcept -> arr_t<Size, T>
    {
        return ice::math::div(left, right);
    }

    template<u32 Size, typename T, typename U>
    constexpr auto operator/(vec<Size, T> left, arr_t<Size, U> right) noexcept -> vec<Size, T>
    {
        return ice::math::div(left, right);
    }

} // namespace ice::math
