#pragma once
#include <inttypes.h>

namespace core
{

    enum class ModuleHandle : std::uintptr_t
    {
        Invalid = 0x0
    };

    using std::addressof;

    template<typename T, uint32_t Size>
    constexpr inline auto size(T(&)[Size]) noexcept
    {
        return Size;
    }

    template<typename T>
    constexpr inline auto hash(T value) noexcept -> uint64_t
    {
        return static_cast<uint64_t>(value);
    }

    struct VariadicExpansionContext
    {
        template<typename...T>
        constexpr VariadicExpansionContext(T&&...) noexcept { }
    };

    template<typename T>
    constexpr auto combine_flag(T left, T right) noexcept
    {
        static_assert(std::is_enum_v<T>, "T is not a valid enum value.");
        auto const left_value = static_cast<std::underlying_type_t<T>>(left);
        auto const right_value = static_cast<std::underlying_type_t<T>>(right);
        return T{ left_value | right_value };
    }

    template<typename T>
    constexpr auto has_flag(T flags, T mask) noexcept
    {
        static_assert(std::is_enum_v<T>, "T is not a valid enum value.");
        auto const flags_value = static_cast<std::underlying_type_t<T>>(flags);
        auto const mask_value = static_cast<std::underlying_type_t<T>>(mask);
        return (flags_value & mask_value) == mask_value;
    }

} // namespace core
