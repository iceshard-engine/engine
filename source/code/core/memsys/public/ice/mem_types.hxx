/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>
#include <ice/concept/strong_type_integral.hxx>

namespace ice
{

    enum class ualign : ice::u32;

    //! \brief Represents a signed size value on the given platform.
    //! \note The 'isize' type is used to force explicit conversions back to 'usize' after performing "unsafe" operations like 'subtraction'.
    struct isize
    {
        using TypeTag = ice::StrongNumeric;
        using base_type = std::conditional_t<ice::build::is_x64, ice::i64, ice::i32>;

        constexpr auto to_usize() const noexcept;

        base_type value;
    };

    //! \brief Represents a unsigned size value on the given platform.
    //! \note Some operations are limited or return a different result type, for ex.: 'usize - usize => isize'
    struct usize
    {
        using TypeTag = ice::StrongNumeric;
        using base_type = std::conditional_t<ice::build::is_x64, ice::u64, ice::u32>;

        constexpr operator isize() const noexcept;

        //! \brief Perform a checked subtraction, ensuring 'left' is greater or equal to 'right'.
        static constexpr auto subtract(ice::usize left, ice::usize right) noexcept -> ice::usize;

        base_type value;
    };

    enum class ualign : ice::u32
    {
        invalid = 0,

        b_1 = 1,
        b_2 = 2,
        b_4 = 4,
        b_8 = 8,
        b_16 = 16,
        b_32 = 32,
        b_64 = 64,
        b_128 = 128,
        b_256 = 256,
        b_512 = 512,
        b_1024 = 1024,
        b_2048 = 2048,

        b_default = ice::build::is_x64 ? b_16 : b_8,
    };

    struct meminfo
    {
        ice::usize size{ 0 };
        ice::ualign alignment = ice::ualign::b_default;
    };

    struct Data;
    struct Memory;

    // MEMORY SIZE LITERALS

    constexpr auto operator""_B(unsigned long long v) noexcept -> ice::usize
    {
        return ice::usize{ static_cast<ice::usize::base_type>(v) };
    }

    constexpr auto operator""_KiB(unsigned long long v) noexcept -> ice::usize
    {
        return ice::usize{ static_cast<ice::usize::base_type>(v) * 1024 };
    }

    constexpr auto operator""_MiB(unsigned long long v) noexcept -> ice::usize
    {
        return ice::usize{ static_cast<ice::usize::base_type>(v) * 1024 * 1024 };
    }

    // EXPLICIT OPERATORS

    constexpr auto isize::to_usize() const noexcept
    {
        return ice::usize{ static_cast<ice::usize::base_type>(value) };
    }

    constexpr usize::operator isize() const noexcept
    {
        return ice::isize{ static_cast<ice::isize::base_type>(value) };
    }

    constexpr auto usize::subtract(ice::usize baseval, ice::usize subval) noexcept -> ice::usize
    {
        ICE_ASSERT_CORE(baseval >= subval);
        return { baseval.value - subval.value };
    }


    constexpr auto operator-(ice::usize left, ice::usize right) noexcept -> ice::isize
    {
        return { static_cast<ice::isize::base_type>(left.value) - static_cast<ice::isize::base_type>(right.value) };
    }

    constexpr auto operator-=(ice::usize& left, ice::usize right) noexcept -> ice::usize& = delete;

    constexpr auto operator-(ice::usize left) noexcept -> ice::isize
    {
        return { -static_cast<ice::isize::base_type>(left.value) };
    }

    constexpr auto operator==(ice::usize left, ice::isize right) noexcept -> bool
    {
        return static_cast<ice::isize::base_type>(left.value) == right.value;
    }

    constexpr auto operator+(ice::usize left, ice::isize right) noexcept -> ice::isize
    {
        return { static_cast<ice::isize::base_type>(left.value) + right.value };
    }

    constexpr auto operator-(ice::usize left, ice::isize right) noexcept -> ice::isize
    {
        return { static_cast<ice::isize::base_type>(left.value) - right.value };
    }

    constexpr auto operator+(ice::isize left, ice::usize right) noexcept -> ice::isize
    {
        return { left.value + static_cast<ice::isize::base_type>(right.value) };
    }

    constexpr auto operator-(ice::isize left, ice::usize right) noexcept -> ice::isize
    {
        return { left.value - static_cast<ice::isize::base_type>(right.value) };
    }

    constexpr auto operator%(ice::usize left, ice::ualign right) noexcept -> ice::usize
    {
        return { left.value % static_cast<ice::usize::base_type>(right) };
    }

    constexpr auto operator%(ice::isize left, ice::ualign right) noexcept -> ice::isize
    {
        return { left.value % static_cast<ice::isize::base_type>(right) };
    }

    constexpr auto operator<=>(ice::usize left, ice::isize right) noexcept
    {
        return static_cast<ice::isize::base_type>(left.value) <=> right.value;
    }

    constexpr auto operator<=>(ice::isize left, ice::usize right) noexcept
    {
        return left.value <=> static_cast<ice::isize::base_type>(right.value);
    }

} // namespace ice
