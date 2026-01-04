/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem.hxx>

namespace ice
{

    struct ncount;
    struct nindex;

    namespace concepts
    {

        template<typename T>
        concept native_count_type = (std::is_arithmetic_v<T> and not std::is_floating_point_v<T>)
            or std::is_same_v<T, ncount>
            or std::is_same_v<T, nindex>;

    } // namespace concepts

    namespace detail
    {

#if ISP_ARCH_BITS == 64
        using ncount_base_type = u64;
        using ncount_base_type_signed = i64;
#elif ISP_ARCH_BITS == 32
        using ncount_base_type = u32;
        using ncount_base_type_signed = i32;
#else
#   error Unhandled architecture!
#endif

        template<
            typename BaseType = ncount_base_type,
            typename BaseTypeSigned = ncount_base_type_signed>
        struct ncount_base;

        template<>
        struct ncount_base<u64, i64>
        {
            using base_type = u64;
            using base_type_signed = i64;

            base_type_signed value : 48 = 0;
            base_type width : 16 = 0;

            constexpr ncount_base() noexcept = default;
            constexpr ncount_base(base_type value) noexcept
                : value{ static_cast<base_type_signed>(value) }, width{ 1 }
            { }
            constexpr explicit ncount_base(base_type value, base_type width) noexcept
                : value{ static_cast<base_type_signed>(value) }, width{ width }
            { }
        };

        template<>
        struct ncount_base<u32, i32>
        {
            using base_type = u32;
            using base_type_signed = i32;

            base_type_signed value = 0;
            base_type width = 0;

            constexpr ncount_base() noexcept = default;
            constexpr ncount_base(base_type value) noexcept
                : value{ static_cast<base_type_signed>(value) }, width{ 1 }
            { }
            constexpr explicit ncount_base(base_type value, base_type width) noexcept
                : value{ static_cast<base_type_signed>(value) }, width{ width }
            { }
        };

        constexpr auto ncount_min_value() noexcept -> ncount_base_type
        {
            return 0;
        }

        constexpr auto ncount_max_value() noexcept -> ncount_base_type
        {
            if constexpr (ice::build::is_x64)
            {
                return 0x0000'7fff'ffff'ffff;
            }
            else
            {
                return std::numeric_limits<ncount_base_type_signed>::max();
            }
        }

    } // namespace detail


    struct ncount : protected ice::detail::ncount_base<>
    {
        using ncount_base<>::base_type;
        using ncount_base<>::ncount_base;

        // Utility
        constexpr auto u8() const noexcept { return static_cast<ice::u8>(value); }
        constexpr auto u16() const noexcept { return static_cast<ice::u16>(value); }
        constexpr auto u32() const noexcept { return static_cast<ice::u32>(value); }
        constexpr auto u64() const noexcept { return static_cast<ice::u64>(value); }
        constexpr auto native() const noexcept { return static_cast<base_type>(value); }
        constexpr auto bytes() const noexcept { return operator ice::usize(); }

        // Arithmetic
        constexpr auto operator+(this ncount self, ice::concepts::native_count_type auto other) noexcept -> ncount;
        constexpr auto operator-(this ncount self, ice::concepts::native_count_type auto other) noexcept -> ncount;
        constexpr auto operator*(this ncount self, ice::concepts::native_count_type auto other) noexcept -> ncount;
        constexpr auto operator/(this ncount self, ice::concepts::native_count_type auto other) noexcept -> ncount;
        constexpr auto operator+=(this ncount& self, ice::concepts::native_count_type auto other) noexcept -> ncount&;
        constexpr auto operator-=(this ncount& self, ice::concepts::native_count_type auto other) noexcept -> ncount&;
        constexpr auto operator*=(this ncount& self, ice::concepts::native_count_type auto other) noexcept -> ncount&;
        constexpr auto operator/=(this ncount& self, ice::concepts::native_count_type auto other) noexcept -> ncount&;

        // Increments
        constexpr auto operator++(this ncount& self) noexcept -> ncount&;
        constexpr auto operator++(this ncount& self, int) noexcept -> ncount;
        constexpr auto operator--(this ncount& self) noexcept -> ncount&;
        constexpr auto operator--(this ncount& self, int) noexcept -> ncount;

        // Comparison
        constexpr bool operator==(this ncount self, ice::concepts::native_count_type auto other) noexcept;
        constexpr auto operator<=>(this ncount self, ice::concepts::native_count_type auto other) noexcept;

        // type colapsing
        constexpr explicit operator bool(this ncount self) noexcept;
        constexpr operator ice::usize(this ncount self) noexcept;
        constexpr operator base_type(this ncount self) noexcept;
        constexpr explicit operator base_type_signed(this ncount self) noexcept;
    };

    struct ncount_invalid_t : ncount {};

    static constexpr ice::ncount_invalid_t none_count{ };
    static constexpr ice::ncount ncount_max{ ice::detail::ncount_max_value() };
    static constexpr ice::ncount ncount_min{ ice::detail::ncount_min_value() };

    static_assert(ice::size_of<ncount> == 8_B);

    constexpr auto ncount::operator+(this ncount self, ice::concepts::native_count_type auto other) noexcept -> ncount
    {
        return ncount{ static_cast<base_type>(self.value + static_cast<base_type_signed>(other)), self.width };
    }

    constexpr auto ncount::operator-(this ncount self, ice::concepts::native_count_type auto other) noexcept -> ncount
    {
        return ncount{ static_cast<base_type>(self.value - static_cast<base_type_signed>(other)), self.width };
    }

    constexpr auto ncount::operator*(this ncount self, ice::concepts::native_count_type auto other) noexcept -> ncount
    {
        return ncount{ static_cast<base_type>(self.value * static_cast<base_type_signed>(other)), self.width };
    }

    constexpr auto ncount::operator/(this ncount self, ice::concepts::native_count_type auto other) noexcept -> ncount
    {
        return ncount{ static_cast<base_type>(self.value / static_cast<base_type_signed>(other)), self.width };
    }

    constexpr auto ncount::operator+=(this ncount& self, ice::concepts::native_count_type auto other) noexcept -> ncount&
    {
        self.value += static_cast<base_type_signed>(other);
        return self;
    }

    constexpr auto ncount::operator-=(this ncount& self, ice::concepts::native_count_type auto other) noexcept -> ncount&
    {
        self.value -= static_cast<base_type_signed>(other);
        return self;
    }

    constexpr auto ncount::operator*=(this ncount& self, ice::concepts::native_count_type auto other) noexcept -> ncount&
    {
        self.value *= static_cast<base_type_signed>(other);
        return self;
    }

    constexpr auto ncount::operator/=(this ncount& self, ice::concepts::native_count_type auto other) noexcept -> ncount&
    {
        self.value /= static_cast<base_type_signed>(other);
        return self;
    }

    constexpr auto ncount::operator++(this ncount& self) noexcept -> ncount&
    {
        self.value += 1;
        return self;
    }

    constexpr auto ncount::operator++(this ncount& self, int) noexcept -> ncount
    {
        const ncount old = self;
        self.value += 1;
        return old;
    }

    constexpr auto ncount::operator--(this ncount& self) noexcept -> ncount&
    {
        self.value -= 1;
        return self;
    }

    constexpr auto ncount::operator--(this ncount& self, int) noexcept -> ncount
    {
        const ncount old = self;
        self.value -= 1;
        return old;
    }

    constexpr bool ncount::operator==(this ncount self, ice::concepts::native_count_type auto other) noexcept
    {
        return self.value == static_cast<base_type_signed>(other);
    }

    constexpr auto ncount::operator<=>(this ncount self, ice::concepts::native_count_type auto other) noexcept
    {
        return self.value <=> static_cast<base_type_signed>(other);
    }

    constexpr ncount::operator bool(this ncount self) noexcept
    {
        return static_cast<bool>(self.value * self.width);
    }

    constexpr ncount::operator ice::usize(this ncount self) noexcept
    {
        return ice::usize{ static_cast<ice::usize::base_type>(self.value) * self.width };
    }

    constexpr ncount::operator ncount::base_type(this ncount self) noexcept
    {
        return std::max<base_type>(self.value, 0);
    }

    constexpr ncount::operator ncount::base_type_signed(this ncount self) noexcept
    {
        return self.value;
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
