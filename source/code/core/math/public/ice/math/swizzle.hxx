/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/math/vector.hxx>

namespace ice::math
{

    namespace detail_math
    {

        template<u8... Components>
        struct swizzle{ };

    } // namespace detail

    template<u32 Size, typename T, u8... Components>
    constexpr auto operator|(mat<Size, 1, T> vec, detail_math::swizzle<Components...>) noexcept -> mat<sizeof...(Components), 1, T>
    {
        return { (Components >= Size ? T{} : vec.v[0][Components])... };
    };

    struct swizzle_definitions
    {

        static constexpr auto xxxx = detail_math::swizzle<0,0,0,0>{};
        static constexpr auto xxx = detail_math::swizzle<0,0,0>{};
        static constexpr auto xx = detail_math::swizzle<0,0>{};
        static constexpr auto x = detail_math::swizzle<0>{};

        static constexpr auto yyyy = detail_math::swizzle<1,1,1,1>{};
        static constexpr auto yyy = detail_math::swizzle<1,1,1>{};
        static constexpr auto yy = detail_math::swizzle<1,1>{};
        static constexpr auto y = detail_math::swizzle<1>{};

        static constexpr auto zzzz = detail_math::swizzle<2,2,2,2>{};
        static constexpr auto zzz = detail_math::swizzle<2,2,2>{};
        static constexpr auto zz = detail_math::swizzle<2,2>{};
        static constexpr auto z = detail_math::swizzle<2>{};

        static constexpr auto wwww = detail_math::swizzle<3,3,3,3>{};
        static constexpr auto www = detail_math::swizzle<3,3,3>{};
        static constexpr auto ww = detail_math::swizzle<3,3>{};
        static constexpr auto w = detail_math::swizzle<3>{};

        static constexpr auto xyzw = detail_math::swizzle<0,1,2,3>{};
        static constexpr auto xyz = detail_math::swizzle<0,1,2>{};
        static constexpr auto xy = detail_math::swizzle<0,1>{};
        static constexpr auto xz = detail_math::swizzle<0,2>{};

        static constexpr auto wzyx = detail_math::swizzle<3,2,1,0>{};
        static constexpr auto zyx = detail_math::swizzle<2,1,0>{};
        static constexpr auto yx = detail_math::swizzle<1,0>{};
        static constexpr auto zx = detail_math::swizzle<2,0>{};

    };

    static constexpr swizzle_definitions sw;

} // namespace ice::math
