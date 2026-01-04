/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem.hxx>

namespace ice
{

    struct nvalue;

    namespace detail
    {

        using nvalue_base_utype = ice::usize::base_type;
        using nvalue_base_stype = ice::isize::base_type;

        // utilities

        consteval auto ncount_min_value() noexcept -> ice::detail::nvalue_base_utype;
        consteval auto ncount_max_value() noexcept -> ice::detail::nvalue_base_utype;

        // Sanity Checks

        static_assert(sizeof(ice::detail::nvalue_base_utype) == sizeof(size_t));
        static_assert(sizeof(ice::detail::nvalue_base_stype) == sizeof(size_t));

    } // namespace detail

    struct ncount;
    struct nindex;

    namespace concepts
    {

        template<typename T>
        concept NValueCompatibleType = (std::is_arithmetic_v<T> and not std::is_floating_point_v<T>)
            or std::is_same_v<T, ice::nvalue>
            or std::is_same_v<T, ice::ncount>
            or std::is_same_v<T, ice::nindex>;

    } // namespace concepts

#if ISP_ARCH_BITS == 64
#define ICE_NVALUE_VALUE_MAX 0x0000'7fff'ffff'ffff
#define ICE_NVALUE_VALUE_FIELD_BITS : 48
#define ICE_NVALUE_WIDTH_FIELD_BITS : 16

    static_assert(sizeof(ice::detail::nvalue_base_utype) == 8);
    static_assert(sizeof(ice::detail::nvalue_base_stype) == 8);
#elif ISP_ARCH_BITS == 32
#define ICE_NVALUE_VALUE_MAX 0xffff'ffff
#define ICE_NVALUE_VALUE_FIELD_BITS
#define ICE_NVALUE_WIDTH_FIELD_BITS

    static_assert(sizeof(ice::detail::nvalue_base_utype) == 4);
    static_assert(sizeof(ice::detail::nvalue_base_stype) == 4);
#else
#   error Unhandled architecture!
#endif

    consteval auto detail::ncount_min_value() noexcept -> ice::detail::nvalue_base_utype
    {
        return 0;
    }

    consteval auto detail::ncount_max_value() noexcept -> ice::detail::nvalue_base_utype
    {
        return ICE_NVALUE_VALUE_MAX;
    }

    struct nvalue
    {
        using base_type = ice::detail::nvalue_base_utype;
        using base_signed_type = ice::detail::nvalue_base_stype;

        // NOTE: The '_width' needs to be defined BEFORE value. So the high-bits (that are accessed less frequently) contain
        //   the '_width' field and the low-bits contain the '_value' field.
        // This is to ensure that the compiler does not need to shift the '_value' part every time an operation is done.
        ice::detail::nvalue_base_utype _width ICE_NVALUE_WIDTH_FIELD_BITS;
        ice::detail::nvalue_base_stype _value ICE_NVALUE_VALUE_FIELD_BITS;

        // Checks if the value is valid. (_width > 0)
        template<typename Self>
        constexpr bool is_valid(this Self self) noexcept { return self._width > 0; }

        template<typename Self>
        constexpr auto value_or(this Self self, ice::concepts::NValueCompatibleType auto fallback) noexcept
        {
            return self.is_valid() ? static_cast<std::remove_reference_t<decltype(fallback)>>(self.native()) : fallback;
        }

        // NOTE: In most cases we will use '_width' as a validation field instead of actually using it's value.
        //   I may come in handy for some operations (ncount -> usize) but it's purpose is to define a concrete 'invalid' state.
        constexpr auto native() const noexcept { return static_cast<base_type>(_value * (_width != 0)); }
        constexpr auto u8() const noexcept { return static_cast<ice::u8>(native()); }
        constexpr auto u16() const noexcept { return static_cast<ice::u16>(native()); }
        constexpr auto u32() const noexcept { return static_cast<ice::u32>(native()); }
        constexpr auto u64() const noexcept { return static_cast<ice::u64>(native()); }

        constexpr auto bytes() const noexcept -> ice::usize { return { static_cast<ice::usize::base_type>(_value) * _width }; }
    };

} // namespace ice
