/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem.hxx>
#include <ice/types/nvalue.hxx>

namespace ice
{

    struct ncount;
    struct nindex;

    struct ncount : public ice::nvalue
    {
        using nvalue::base_type;
        using nvalue::base_signed_type;

        constexpr ncount() noexcept = default;
        constexpr ncount(nvalue value) noexcept;
        constexpr ncount(base_type value) noexcept;
        constexpr ncount(base_type value, base_type width) noexcept;

        // Arithmetic
        constexpr auto operator+(this ncount self, ice::concepts::NValueCompatibleType auto other) noexcept -> ncount;
        constexpr auto operator-(this ncount self, ice::concepts::NValueCompatibleType auto other) noexcept -> ncount;
        constexpr auto operator*(this ncount self, ice::concepts::NValueCompatibleType auto other) noexcept -> ncount;
        constexpr auto operator/(this ncount self, ice::concepts::NValueCompatibleType auto other) noexcept -> ncount;
        constexpr auto operator+=(this ncount& self, ice::concepts::NValueCompatibleType auto other) noexcept -> ncount&;
        constexpr auto operator-=(this ncount& self, ice::concepts::NValueCompatibleType auto other) noexcept -> ncount&;
        constexpr auto operator*=(this ncount& self, ice::concepts::NValueCompatibleType auto other) noexcept -> ncount&;
        constexpr auto operator/=(this ncount& self, ice::concepts::NValueCompatibleType auto other) noexcept -> ncount&;

        // Increments
        constexpr auto operator++(this ncount& self) noexcept -> ncount&;
        constexpr auto operator++(this ncount& self, int) noexcept -> ncount;
        constexpr auto operator--(this ncount& self) noexcept -> ncount&;
        constexpr auto operator--(this ncount& self, int) noexcept -> ncount;

        // Comparison
        constexpr bool operator==(this ncount self, ice::concepts::NValueCompatibleType auto other) noexcept;
        constexpr auto operator<=>(this ncount self, ice::concepts::NValueCompatibleType auto other) noexcept;

        // type colapsing
        constexpr explicit operator bool(this ncount self) noexcept;
        constexpr operator ice::usize(this ncount self) noexcept;
        constexpr operator base_type(this ncount self) noexcept;
        constexpr explicit operator base_signed_type(this ncount self) noexcept;
    };

    constexpr ncount::ncount(nvalue value) noexcept
        : nvalue{ value }
    { }

    constexpr ncount::ncount(base_type value) noexcept
        : nvalue{ 1, static_cast<base_signed_type>(value) }
    { }

    inline constexpr ncount::ncount(base_type value, base_type width) noexcept
        : nvalue{ width, static_cast<base_signed_type>(value) }
    { }

    struct ncount_invalid_t : ncount {};

    static constexpr ice::ncount_invalid_t none_count{ };
    static constexpr ice::ncount ncount_max{ ice::detail::ncount_max_value() };
    static constexpr ice::ncount ncount_min{ ice::detail::ncount_min_value() };

    static_assert(ice::size_of<ncount> == 8_B);

    constexpr auto ncount::operator+(this ncount self, ice::concepts::NValueCompatibleType auto other) noexcept -> ncount
    {
        return ncount{ static_cast<base_type>(self._value + static_cast<base_signed_type>(other)), self._width };
    }

    constexpr auto ncount::operator-(this ncount self, ice::concepts::NValueCompatibleType auto other) noexcept -> ncount
    {
        return ncount{ static_cast<base_type>(self._value - static_cast<base_signed_type>(other)), self._width };
    }

    constexpr auto ncount::operator*(this ncount self, ice::concepts::NValueCompatibleType auto other) noexcept -> ncount
    {
        return ncount{ static_cast<base_type>(self._value * static_cast<base_signed_type>(other)), self._width };
    }

    constexpr auto ncount::operator/(this ncount self, ice::concepts::NValueCompatibleType auto other) noexcept -> ncount
    {
        return ncount{ static_cast<base_type>(self._value / static_cast<base_signed_type>(other)), self._width };
    }

    constexpr auto ncount::operator+=(this ncount& self, ice::concepts::NValueCompatibleType auto other) noexcept -> ncount&
    {
        self._value += static_cast<base_signed_type>(other);
        return self;
    }

    constexpr auto ncount::operator-=(this ncount& self, ice::concepts::NValueCompatibleType auto other) noexcept -> ncount&
    {
        self._value -= static_cast<base_signed_type>(other);
        return self;
    }

    constexpr auto ncount::operator*=(this ncount& self, ice::concepts::NValueCompatibleType auto other) noexcept -> ncount&
    {
        self._value *= static_cast<base_signed_type>(other);
        return self;
    }

    constexpr auto ncount::operator/=(this ncount& self, ice::concepts::NValueCompatibleType auto other) noexcept -> ncount&
    {
        self._value /= static_cast<base_signed_type>(other);
        return self;
    }

    constexpr auto ncount::operator++(this ncount& self) noexcept -> ncount&
    {
        self._value += 1;
        return self;
    }

    constexpr auto ncount::operator++(this ncount& self, int) noexcept -> ncount
    {
        const ncount old = self;
        self._value += 1;
        return old;
    }

    constexpr auto ncount::operator--(this ncount& self) noexcept -> ncount&
    {
        self._value -= 1;
        return self;
    }

    constexpr auto ncount::operator--(this ncount& self, int) noexcept -> ncount
    {
        const ncount old = self;
        self._value -= 1;
        return old;
    }

    constexpr bool ncount::operator==(this ncount self, ice::concepts::NValueCompatibleType auto other) noexcept
    {
        return self._value == static_cast<base_signed_type>(other);
    }

    constexpr auto ncount::operator<=>(this ncount self, ice::concepts::NValueCompatibleType auto other) noexcept
    {
        return self._value <=> static_cast<base_signed_type>(other);
    }

    constexpr ncount::operator bool(this ncount self) noexcept
    {
        return static_cast<bool>(self._value * self._width);
    }

    constexpr ncount::operator ice::usize(this ncount self) noexcept
    {
        return ice::usize{ static_cast<ice::usize::base_type>(self._value) * self._width };
    }

    constexpr ncount::operator ncount::base_type(this ncount self) noexcept
    {
        return std::max<base_type>(self._value, 0);
    }

    constexpr ncount::operator ncount::base_signed_type(this ncount self) noexcept
    {
        return self._value;
    }

    // special operators

    constexpr bool operator==(ncount self, ncount_invalid_t) noexcept
    {
        return static_cast<bool>(self) == false;
    }

    template<typename CharType> requires (std::is_same_v<CharType, char> || std::is_same_v<CharType, wchar_t>)
    constexpr auto operator+(CharType* char_ptr, ncount count) noexcept -> CharType*
    {
        return char_ptr + count.u64();
    }

    template<typename CharType> requires (std::is_same_v<CharType, char> || std::is_same_v<CharType, wchar_t>)
    constexpr auto operator+(CharType const* char_ptr, ncount count) noexcept -> CharType const*
    {
        return char_ptr + count.u64();
    }

} // namespace ice
