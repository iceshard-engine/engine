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

        consteval auto nvalue_min_value() noexcept -> ice::detail::nvalue_base_utype;
        consteval auto nvalue_max_value() noexcept -> ice::detail::nvalue_base_utype;

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

    consteval auto detail::nvalue_min_value() noexcept -> ice::detail::nvalue_base_utype
    {
        return 0;
    }

    consteval auto detail::nvalue_max_value() noexcept -> ice::detail::nvalue_base_utype
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

        // Checks if the value is valid. (_width != 0)
        template<typename Self>
        constexpr bool is_valid(this Self self) noexcept { return static_cast<bool>(self._width); }

        template<typename Self>
        constexpr auto value_or(this Self self, ice::concepts::NValueCompatibleType auto fallback) noexcept
        {
            return self.is_valid() ? static_cast<std::remove_reference_t<decltype(fallback)>>(self.native()) : fallback;
        }

        template<typename Self>
        constexpr auto min_value_or(
            this Self self,
            ice::concepts::NValueCompatibleType auto other,
            ice::concepts::NValueCompatibleType auto fallback
        ) noexcept
        {
            using ResultType = std::remove_reference_t<decltype(fallback)>;

            ResultType const second_value = static_cast<ResultType>(other);
            return self.is_valid() ? ice::min<ResultType>(static_cast<ResultType>(self.native()), second_value) : fallback;
        }

        // NOTE: In most cases we will use '_width' as a validation field instead of actually using it's value.
        //   I may come in handy for some operations (ncount -> usize) but it's purpose is to define a concrete 'invalid' state.
        constexpr auto native() const noexcept { return static_cast<base_type>(_value * (_width != 0)); }
        constexpr auto u8() const noexcept { return static_cast<ice::u8>(native()); }
        constexpr auto u16() const noexcept { return static_cast<ice::u16>(native()); }
        constexpr auto u32() const noexcept { return static_cast<ice::u32>(native()); }
        constexpr auto u64() const noexcept { return static_cast<ice::u64>(native()); }

        // Allow 'nvalue' types collaps to the 'base_type'
        constexpr operator base_type(this nvalue self) noexcept { return self.native(); }

        // Equality is has two cases:
        // > when deriving from 'nvalue':
        //   - both sides need to be either invalid or valid and have the same '_value'.
        //   - the '_width' can be different.
        // > when comparing to a regular number:
        //   - the 'nvalue' needs to be valid and the number has the the same value as '_value'
        template<typename Self>
        constexpr bool operator==(this Self self, ice::concepts::NValueCompatibleType auto other) noexcept;

        template<typename Self>
        constexpr auto operator<=>(
            this Self self, ice::concepts::NValueCompatibleType auto other
        ) noexcept -> std::strong_ordering;

        // Increments
        template<typename Self>
        constexpr auto operator++(this Self& self) noexcept -> Self&;

        template<typename Self>
        constexpr auto operator++(this Self& self, int) noexcept -> Self;

        template<typename Self>
        constexpr auto operator--(this Self& self) noexcept -> Self&;

        template<typename Self>
        constexpr auto operator--(this Self& self, int) noexcept -> Self;

        // Arithmetics
        template<typename Self>
        constexpr auto operator+(this Self self, ice::concepts::NValueCompatibleType auto other) noexcept -> Self;

        template<typename Self>
        constexpr auto operator-(this Self self, ice::concepts::NValueCompatibleType auto other) noexcept -> Self;

        template<typename Self>
        constexpr auto operator*(this Self self, ice::concepts::NValueCompatibleType auto other) noexcept -> Self;

        template<typename Self>
        constexpr auto operator/(this Self self, ice::concepts::NValueCompatibleType auto other) noexcept -> Self;

        template<typename Self>
        constexpr auto operator+=(this Self& self, ice::concepts::NValueCompatibleType auto other) noexcept -> Self&;

        template<typename Self>
        constexpr auto operator-=(this Self& self, ice::concepts::NValueCompatibleType auto other) noexcept -> Self&;

        template<typename Self>
        constexpr auto operator*=(this Self& self, ice::concepts::NValueCompatibleType auto other) noexcept -> Self&;

        template<typename Self>
        constexpr auto operator/=(this Self& self, ice::concepts::NValueCompatibleType auto other) noexcept -> Self&;
    };

    template<typename Self>
    inline constexpr bool nvalue::operator==(
        this Self self, ice::concepts::NValueCompatibleType auto other
    ) noexcept
    {
        if constexpr (std::is_base_of_v<ice::nvalue, decltype(other)>)
        {
#if 0 // The naive approach (contains jump instructions even after optimization)
            return self.is_valid() == other.is_valid() && (self.is_valid() == false || self.native() == other.native());
#else // The mathematical approach (jump instructions are not present, branch predictor is happy)
            return static_cast<bool>((self._width * other._width) * (self._value == other._value) + ((self._width + other._width) == 0));
#endif
        }
        else
        {
            return self.native() == other;
        }
    }

    //           (nvalue{W, V} == nvalue{W, V});
    static_assert(nvalue{0, 0} == nvalue{0, 0}, "Invalid values are equal to each other");
    static_assert(nvalue{0, 1} == nvalue{0, 1}, "Invalid values are equal to each other ('_value' is not '0')");
    static_assert(nvalue{1, 0} == nvalue{1, 0}, "Valid values are equal if '_value' is the same.");
    static_assert(nvalue{1, 4} == nvalue{2, 4}, "Valid values are equal if '_value' is the same. ('_width' differs)");
    static_assert(nvalue{1, 0} != nvalue{0, 0}, "Valid values are not equal to invalid values. ('_value' is '0' in both)");
    static_assert(nvalue{1, 0} != nvalue{1, 1}, "Valid values are not equal if '_value' differs ('_width' is '1' in both)");
    static_assert(nvalue{1, 1} != nvalue{2, 2}, "Valid values are not equal if '_value' differs ('_width' and '_value' differs)");

    template<typename Self>
    inline constexpr auto nvalue::operator<=>(
        this Self self, ice::concepts::NValueCompatibleType auto other
    ) noexcept -> std::strong_ordering
    {
        if constexpr (std::is_base_of_v<ice::nvalue, decltype(other)>)
        {
            return self.native() <=> other.native();
        }
        else
        {
            return self.native() <=> static_cast<ice::detail::nvalue_base_stype>(other);
        }
    }

    // Increments
    template<typename Self>
    constexpr auto nvalue::operator++(this Self& self) noexcept -> Self&
    {
        self._value += 1;
        return self;
    }

    template<typename Self>
    constexpr auto nvalue::operator++(this Self& self, int) noexcept -> Self
    {
        const Self old = self;
        self._value += 1;
        return old;
    }

    template<typename Self>
    constexpr auto nvalue::operator--(this Self& self) noexcept -> Self&
    {
        self._value -= 1;
        return self;
    }

    template<typename Self>
    constexpr auto nvalue::operator--(this Self& self, int) noexcept -> Self
    {
        const Self old = self;
        self._value -= 1;
        return old;
    }

    // Arithmetics
    template<typename Self>
    inline constexpr auto nvalue::operator+(
        this Self self, ice::concepts::NValueCompatibleType auto other
    ) noexcept -> Self
    {
        return Self{ ice::nvalue{ self._width, self._value + static_cast<ice::detail::nvalue_base_stype>(other) } };
    }

    template<typename Self>
    inline constexpr auto nvalue::operator-(
        this Self self, ice::concepts::NValueCompatibleType auto other
    ) noexcept -> Self
    {
        return Self{ ice::nvalue{ self._width, self._value - static_cast<ice::detail::nvalue_base_stype>(other) } };
    }

    template<typename Self>
    inline constexpr auto nvalue::operator*(
        this Self self, ice::concepts::NValueCompatibleType auto other
    ) noexcept -> Self
    {
        return Self{ ice::nvalue{ self._width, self._value * static_cast<ice::detail::nvalue_base_stype>(other) } };
    }

    template<typename Self>
    inline constexpr auto nvalue::operator/(
        this Self self, ice::concepts::NValueCompatibleType auto other
    ) noexcept -> Self
    {
        return Self{ ice::nvalue{ self._width, self._value / static_cast<ice::detail::nvalue_base_stype>(other) } };
    }

    template<typename Self>
    inline constexpr auto nvalue::operator+=(
        this Self& self, ice::concepts::NValueCompatibleType auto other
    ) noexcept -> Self&
    {
        self._value += static_cast<ice::detail::nvalue_base_stype>(other);
        return self;
    }

    template<typename Self>
    inline constexpr auto nvalue::operator-=(
        this Self& self, ice::concepts::NValueCompatibleType auto other
    ) noexcept -> Self&
    {
        self._value -= static_cast<ice::detail::nvalue_base_stype>(other);
        return self;
    }

    template<typename Self>
    inline constexpr auto nvalue::operator*=(
        this Self& self, ice::concepts::NValueCompatibleType auto other
    ) noexcept -> Self&
    {
        self._value *= static_cast<ice::detail::nvalue_base_stype>(other);
        return self;
    }

    template<typename Self>
    inline constexpr auto nvalue::operator/=(
        this Self& self, ice::concepts::NValueCompatibleType auto other
    ) noexcept -> Self&
    {
        self._value /= static_cast<ice::detail::nvalue_base_stype>(other);
        return self;
    }

} // namespace ice
