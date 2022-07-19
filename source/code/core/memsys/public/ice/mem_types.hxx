#pragma once
#include <ice/base.hxx>
#include <ice/concept/strong_type_integral.hxx>

namespace ice
{

    enum class ualign : ice::u32;

    struct isize
    {
        using TypeTag = ice::StrongNumeric;
        using base_type = std::conditional_t<ice::build::is_x64, ice::i64, ice::i32>;

        base_type value;
    };

    struct usize
    {
        using TypeTag = ice::StrongNumeric;
        using base_type = std::conditional_t<ice::build::is_x64, ice::u64, ice::u32>;

        constexpr operator isize() noexcept;

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

    constexpr usize::operator isize() noexcept
    {
        return ice::isize{ static_cast<ice::isize::base_type>(value) };
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

} // namespace ice
