/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/types/ncount.hxx>

namespace ice
{

    struct nindex : protected ice::detail::ncount_base<>
    {
        using ncount_base<>::base_type;
        using ncount_base<>::ncount_base;

        constexpr nindex(ice::ncount count) noexcept
            : nindex{ count.u64() }
        { }

        // Helpers
        constexpr bool is_valid(this nindex self) noexcept;

        constexpr auto value_or(this nindex self, ice::concepts::native_count_type auto fallback) noexcept -> ice::nindex;

        constexpr auto u8(this nindex self) noexcept { return static_cast<ice::u8>(self.value * self.width); }
        constexpr auto u16(this nindex self) noexcept { return static_cast<ice::u16>(self.value * self.width); }
        constexpr auto u32(this nindex self) noexcept { return static_cast<ice::u32>(self.value * self.width); }
        constexpr auto u64(this nindex self) noexcept { return static_cast<ice::u64>(self.value * self.width); }
        constexpr auto native(this nindex self) noexcept -> base_type { return static_cast<ice::u64>(self.value * self.width); }

        // Arithmetic
        constexpr auto operator+(this nindex self, ice::concepts::native_count_type auto other) noexcept -> nindex;
        constexpr auto operator-(this nindex self, ice::concepts::native_count_type auto other) noexcept -> nindex;
        constexpr auto operator*(this nindex self, ice::concepts::native_count_type auto other) noexcept -> nindex;
        constexpr auto operator/(this nindex self, ice::concepts::native_count_type auto other) noexcept -> nindex;
        constexpr auto operator+=(this nindex& self, ice::concepts::native_count_type auto other) noexcept -> nindex&;
        constexpr auto operator-=(this nindex& self, ice::concepts::native_count_type auto other) noexcept -> nindex&;
        constexpr auto operator*=(this nindex& self, ice::concepts::native_count_type auto other) noexcept -> nindex&;
        constexpr auto operator/=(this nindex& self, ice::concepts::native_count_type auto other) noexcept -> nindex&;

        // Increments
        constexpr auto operator++(this nindex& self) noexcept -> nindex&;
        constexpr auto operator++(this nindex& self, int) noexcept -> nindex;
        constexpr auto operator--(this nindex& self) noexcept -> nindex&;
        constexpr auto operator--(this nindex& self, int) noexcept -> nindex;

        // Comparison
        constexpr bool operator==(this nindex self, ice::concepts::native_count_type auto other) noexcept;
        constexpr auto operator<=>(this nindex self, ice::concepts::native_count_type auto other) noexcept;

        // type colapsing
        constexpr explicit operator bool(this nindex self) noexcept;
        constexpr operator ice::isize(this nindex self) noexcept;
        constexpr operator ice::ncount(this nindex self) noexcept;
        constexpr operator base_type(this nindex self) noexcept;
        constexpr explicit operator base_type_signed(this nindex self) noexcept;
    };

    struct nindex_invalid_t : nindex {};

    static constexpr ice::nindex_invalid_t none_index{ };

    static_assert(ice::size_of<nindex> == 8_B);

    constexpr bool nindex::is_valid(this nindex self) noexcept
    {
        return self.width > 0;
    }

    constexpr auto nindex::value_or(this nindex self, ice::concepts::native_count_type auto fallback) noexcept -> ice::nindex
    {
        return self.is_valid() ? self : nindex{ static_cast<base_type>(fallback) };
    }

    constexpr auto nindex::operator+(this nindex self, ice::concepts::native_count_type auto other) noexcept -> nindex
    {
        return nindex{ self.value + static_cast<base_type>(other), self.width };
    }

    constexpr auto nindex::operator-(this nindex self, ice::concepts::native_count_type auto other) noexcept -> nindex
    {
        return nindex{ self.value - static_cast<base_type>(other), self.width };
    }

    constexpr auto nindex::operator*(this nindex self, ice::concepts::native_count_type auto other) noexcept -> nindex
    {
        return nindex{ self.value * static_cast<base_type>(other), self.width };
    }

    constexpr auto nindex::operator/(this nindex self, ice::concepts::native_count_type auto other) noexcept -> nindex
    {
        return nindex{ self.value / static_cast<base_type>(other), self.width };
    }

    constexpr auto nindex::operator+=(this nindex& self, ice::concepts::native_count_type auto other) noexcept -> nindex&
    {
        self.value += static_cast<base_type>(other);
        return self;
    }

    constexpr auto nindex::operator-=(this nindex& self, ice::concepts::native_count_type auto other) noexcept -> nindex&
    {
        self.value -= static_cast<base_type>(other);
        return self;
    }

    constexpr auto nindex::operator*=(this nindex& self, ice::concepts::native_count_type auto other) noexcept -> nindex&
    {
        self.value *= static_cast<base_type>(other);
        return self;
    }

    constexpr auto nindex::operator/=(this nindex& self, ice::concepts::native_count_type auto other) noexcept -> nindex&
    {
        self.value /= static_cast<base_type>(other);
        return self;
    }

    constexpr auto nindex::operator++(this nindex& self) noexcept -> nindex&
    {
        self.value += 1;
        return self;
    }

    constexpr auto nindex::operator++(this nindex& self, int) noexcept -> nindex
    {
        const nindex old = self;
        self.value += 1;
        return old;
    }

    constexpr auto nindex::operator--(this nindex& self) noexcept -> nindex&
    {
        self.value -= 1;
        return self;
    }

    constexpr auto nindex::operator--(this nindex& self, int) noexcept -> nindex
    {
        const nindex old = self;
        self.value -= 1;
        return old;
    }

    constexpr bool nindex::operator==(this nindex self, ice::concepts::native_count_type auto other) noexcept
    {
        return static_cast<base_type>(self.value) == static_cast<base_type>(other);
    }

    constexpr auto nindex::operator<=>(this nindex self, ice::concepts::native_count_type auto other) noexcept
    {
        return static_cast<base_type>(self.value) <=> static_cast<base_type>(other);
    }

    constexpr nindex::operator bool(this nindex self) noexcept
    {
        return static_cast<bool>(self.value * self.width);
    }

    constexpr nindex::operator ice::isize(this nindex self) noexcept
    {
        return ice::isize{ static_cast<ice::isize::base_type>(self.value * self.width) };
    }

    constexpr nindex::operator ice::ncount(this nindex self) noexcept
    {
        return ice::ncount{ static_cast<ice::ncount::base_type>(self.value), self.width };
    }

    constexpr nindex::operator nindex::base_type(this nindex self) noexcept
    {
        return std::max<base_type>(self.value, 0);
    }

    constexpr nindex::operator nindex::base_type_signed(this nindex self) noexcept
    {
        return std::max<base_type_signed>(self.value, 0);
    }

    // special operators

    constexpr bool operator==(nindex self, nindex_invalid_t) noexcept
    {
        return self.is_valid() == false;
    }

    template<typename CharType> requires (std::is_same_v<CharType, char> || std::is_same_v<CharType, wchar_t>)
    constexpr auto operator+(CharType* char_ptr, nindex index) noexcept -> CharType*
    {
        return char_ptr + index.u64();
    }

    template<typename CharType> requires (std::is_same_v<CharType, char> || std::is_same_v<CharType, wchar_t>)
    constexpr auto operator+(CharType const* char_ptr, nindex index) noexcept -> CharType const*
    {
        return char_ptr + index.u64();
    }

} // namespace ice
