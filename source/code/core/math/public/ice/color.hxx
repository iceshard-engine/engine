/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/math.hxx>

namespace ice
{

    template<typename T = ice::u8>
    struct Color;

    constexpr auto operator""_argb(unsigned long long value) noexcept -> ice::Color<u8>;
    constexpr auto operator""_abgr(unsigned long long value) noexcept -> ice::Color<u8>;
    constexpr auto operator""_rgba(unsigned long long value) noexcept -> ice::Color<u8>;
    constexpr auto operator""_bgra(unsigned long long value) noexcept -> ice::Color<u8>;

    template<typename T>
    struct Color
    {
        union
        {
            struct /*rgba*/ { T r, g, b, a; };
            struct /*xyzw*/ { T x, y, z, w; };
        };

        constexpr Color() noexcept;
        constexpr Color(Color const&) noexcept = default;

        template<typename Other>
        constexpr Color(Color<Other> const& other) noexcept;

        constexpr explicit Color(ice::u32 rgba) noexcept;
        constexpr explicit Color(ice::u8 r, ice::u8 g, ice::u8 b, ice::u8 a = 255) noexcept;
    };

    namespace detail
    {

        template<ice::u8 t3, ice::u8 t2, ice::u8 t1, ice::u8 t0>
        static constexpr auto color_shuffle8(ice::u64 from) noexcept -> ice::u32
        {
            return (((from >> (t0 * 8)) & 0xff) << 0)
                | (((from >> (t1 * 8)) & 0xff) << 8)
                | (((from >> (t2 * 8)) & 0xff) << 16)
                | (((from >> (t3 * 8)) & 0xff) << 24);
        }

        template<ice::u8 pA, ice::u8 pR, ice::u8 pG, ice::u8 pB>
        static constexpr auto color_from(ice::u64 from) noexcept -> ice::u32
        {
            return color_shuffle8<pA, pR, pG, pB>(from);
        }

        template<typename From, typename To>
        static constexpr auto color_convert(From from) noexcept -> To
        {
            if constexpr (std::is_same_v<From, To>)
            {
                return from;
            }
            else if constexpr (std::is_floating_point_v<From> == false && std::is_floating_point_v<To> == false)
            {
                ice::f32 const to_f32 = static_cast<ice::f32>(static_cast<ice::f32>(from) / static_cast<ice::f32>(std::numeric_limits<From>::max()));
                return static_cast<To>(static_cast<From>(std::numeric_limits<To>::max()) * to_f32 + 0.5f);
            }
            else if constexpr (std::is_same_v<To, ice::u8> || std::is_same_v<To, ice::u16>)
            {
                return static_cast<To>(static_cast<From>(std::numeric_limits<To>::max()) * from + 0.5f);
            }
            else if constexpr (std::is_same_v<From, ice::u8> || std::is_same_v<From, ice::u16>)
            {
                return static_cast<To>(static_cast<To>(from) / static_cast<To>(std::numeric_limits<From>::max()));
            }
            else
            {
                ICE_ASSERT_CORE(false);
                return {};
            }
        }

    } // namespace detail

    template<typename T>
    constexpr Color<T>::Color() noexcept
        : r{ }, g{ }, b{ }, a{ }
    { }

    template<typename T>
    template<typename Other>
    constexpr Color<T>::Color(Color<Other> const& other) noexcept
        : r{ ice::detail::color_convert<Other, T>(other.r) }
        , g{ ice::detail::color_convert<Other, T>(other.g) }
        , b{ ice::detail::color_convert<Other, T>(other.b) }
        , a{ ice::detail::color_convert<Other, T>(other.a) }
    {
    }

    template<typename T>
    constexpr Color<T>::Color(ice::u32 argb) noexcept
        : r{ ice::detail::color_convert<u8, T>((argb >> 16) & 0xff) }
        , g{ ice::detail::color_convert<u8, T>((argb >> 8) & 0xff) }
        , b{ ice::detail::color_convert<u8, T>((argb >> 0) & 0xff) }
        , a{ ice::detail::color_convert<u8, T>((argb >> 24) & 0xff) }
    { }

    constexpr auto operator""_argb(unsigned long long value) noexcept -> ice::Color<u8>
    {
        return ice::Color<u8>{ detail::color_from<3,2,1,0>(value) };
    }

    constexpr auto operator""_abgr(unsigned long long value) noexcept -> ice::Color<u8>
    {
        return ice::Color<u8>{ detail::color_from<3,0,1,2>(value) };
    }

    constexpr auto operator""_rgba(unsigned long long value) noexcept -> ice::Color<u8>
    {
        return ice::Color<u8>{ detail::color_from<0,3,2,1>(value) };
    }

    constexpr auto operator""_bgra(unsigned long long value) noexcept -> ice::Color<u8>
    {
        return ice::Color<u8>{ detail::color_from<0,1,2,3>(value) };
    }

} // namespace ice
