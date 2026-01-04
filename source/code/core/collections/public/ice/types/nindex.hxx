/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/types/ncount.hxx>

namespace ice
{

    struct nindex : public ice::nvalue
    {
        using nvalue::base_type;
        using nvalue::base_signed_type;

        constexpr nindex() noexcept = default;
        constexpr nindex(nvalue value) noexcept;
        constexpr nindex(base_type value) noexcept;
        constexpr nindex(base_type value, base_type width) noexcept;


        // Arithmetic
        constexpr auto operator+(this nindex self, ice::concepts::NValueCompatibleType auto other) noexcept -> nindex;
        constexpr auto operator-(this nindex self, ice::concepts::NValueCompatibleType auto other) noexcept -> nindex;
        constexpr auto operator*(this nindex self, ice::concepts::NValueCompatibleType auto other) noexcept -> nindex;
        constexpr auto operator/(this nindex self, ice::concepts::NValueCompatibleType auto other) noexcept -> nindex;
        constexpr auto operator+=(this nindex& self, ice::concepts::NValueCompatibleType auto other) noexcept -> nindex&;
        constexpr auto operator-=(this nindex& self, ice::concepts::NValueCompatibleType auto other) noexcept -> nindex&;
        constexpr auto operator*=(this nindex& self, ice::concepts::NValueCompatibleType auto other) noexcept -> nindex&;
        constexpr auto operator/=(this nindex& self, ice::concepts::NValueCompatibleType auto other) noexcept -> nindex&;

        // Increments
        constexpr auto operator++(this nindex& self) noexcept -> nindex&;
        constexpr auto operator++(this nindex& self, int) noexcept -> nindex;
        constexpr auto operator--(this nindex& self) noexcept -> nindex&;
        constexpr auto operator--(this nindex& self, int) noexcept -> nindex;

        // Comparison
        constexpr bool operator==(this nindex self, ice::concepts::NValueCompatibleType auto other) noexcept;
        constexpr auto operator<=>(this nindex self, ice::concepts::NValueCompatibleType auto other) noexcept;

        // type colapsing
        constexpr explicit operator bool(this nindex self) noexcept;
        constexpr operator ice::isize(this nindex self) noexcept;
        constexpr operator base_type(this nindex self) noexcept;
        constexpr explicit operator base_signed_type(this nindex self) noexcept;
    };

    constexpr nindex::nindex(nvalue value) noexcept
        : nvalue{ value }
    { }

    constexpr nindex::nindex(base_type value) noexcept
        : nvalue{ 1, static_cast<base_signed_type>(value) }
    { }

    constexpr nindex::nindex(base_type value, base_type width) noexcept
        : nvalue{ width, static_cast<base_signed_type>(value) }
    { }

    struct nindex_invalid_t : nindex { };

    static constexpr ice::nindex_invalid_t none_index{ };

    static_assert(ice::size_of<nindex> == 8_B);

    constexpr auto nindex::operator+(this nindex self, ice::concepts::NValueCompatibleType auto other) noexcept -> nindex
    {
        return nindex{ self._value + static_cast<base_type>(other), self._width };
    }

    constexpr auto nindex::operator-(this nindex self, ice::concepts::NValueCompatibleType auto other) noexcept -> nindex
    {
        return nindex{ self._value - static_cast<base_type>(other), self._width };
    }

    constexpr auto nindex::operator*(this nindex self, ice::concepts::NValueCompatibleType auto other) noexcept -> nindex
    {
        return nindex{ self._value * static_cast<base_type>(other), self._width };
    }

    constexpr auto nindex::operator/(this nindex self, ice::concepts::NValueCompatibleType auto other) noexcept -> nindex
    {
        return nindex{ self._value / static_cast<base_type>(other), self._width };
    }

    constexpr auto nindex::operator+=(this nindex& self, ice::concepts::NValueCompatibleType auto other) noexcept -> nindex&
    {
        self._value += static_cast<base_type>(other);
        return self;
    }

    constexpr auto nindex::operator-=(this nindex& self, ice::concepts::NValueCompatibleType auto other) noexcept -> nindex&
    {
        self._value -= static_cast<base_type>(other);
        return self;
    }

    constexpr auto nindex::operator*=(this nindex& self, ice::concepts::NValueCompatibleType auto other) noexcept -> nindex&
    {
        self._value *= static_cast<base_type>(other);
        return self;
    }

    constexpr auto nindex::operator/=(this nindex& self, ice::concepts::NValueCompatibleType auto other) noexcept -> nindex&
    {
        self._value /= static_cast<base_type>(other);
        return self;
    }

    constexpr auto nindex::operator++(this nindex& self) noexcept -> nindex&
    {
        self._value += 1;
        return self;
    }

    constexpr auto nindex::operator++(this nindex& self, int) noexcept -> nindex
    {
        const nindex old = self;
        self._value += 1;
        return old;
    }

    constexpr auto nindex::operator--(this nindex& self) noexcept -> nindex&
    {
        self._value -= 1;
        return self;
    }

    constexpr auto nindex::operator--(this nindex& self, int) noexcept -> nindex
    {
        const nindex old = self;
        self._value -= 1;
        return old;
    }

    constexpr bool nindex::operator==(this nindex self, ice::concepts::NValueCompatibleType auto other) noexcept
    {
        return static_cast<base_type>(self._value) == static_cast<base_type>(other);
    }

    constexpr auto nindex::operator<=>(this nindex self, ice::concepts::NValueCompatibleType auto other) noexcept
    {
        return static_cast<base_type>(self._value) <=> static_cast<base_type>(other);
    }

    constexpr nindex::operator bool(this nindex self) noexcept
    {
        return static_cast<bool>(self._value * self._width);
    }

    constexpr nindex::operator ice::isize(this nindex self) noexcept
    {
        return ice::isize{ static_cast<ice::isize::base_type>(self._value * self._width) };
    }

    constexpr nindex::operator nindex::base_type(this nindex self) noexcept
    {
        return std::max<base_type>(self._value, 0);
    }

    constexpr nindex::operator nindex::base_signed_type(this nindex self) noexcept
    {
        return std::max<base_signed_type>(self._value, 0);
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
