#pragma once
#include <ice/base.hxx>

namespace ice
{

    enum class ResourceFlags : ice::u32
    {
        None,

        // We reserve 4 bits for quality flags / values.
        Quality_Highest = 0x0000'0001,
        Quality_High = 0x0000'0002,
        Quality_Medium = 0x0000'0003,
        Quality_Low = 0x0000'0004,
        Quality_Lowest = 0x0000'0005,

        Status_Baked = 0x0000'0008,

        // 28 bits to make use of. Please update this enum when this changes.
        First_UnusedValue = 0x0000'0010,

        // Masks to be used to help with flag reading.
        Mask_Quality = 0x0000'000F,
    };

    static constexpr auto operator|(ice::ResourceFlags left, ice::ResourceFlags right) noexcept -> ice::ResourceFlags
    {
        ice::u32 const left_value = static_cast<ice::u32>(left);
        ice::u32 const right_value = static_cast<ice::u32>(left);
        return static_cast<ice::ResourceFlags>(left_value | right_value);
    }

    static constexpr auto operator|=(ice::ResourceFlags& left, ice::ResourceFlags right) noexcept -> ice::ResourceFlags&
    {
        left = left | right;
        return left;
    }

    static constexpr auto operator&(ice::ResourceFlags left, ice::ResourceFlags right) noexcept -> ice::ResourceFlags
    {
        ice::u32 const left_value = static_cast<ice::u32>(left);
        ice::u32 const right_value = static_cast<ice::u32>(left);
        return static_cast<ice::ResourceFlags>(left_value & right_value);
    }

    static constexpr auto operator&=(ice::ResourceFlags& left, ice::ResourceFlags right) noexcept -> ice::ResourceFlags&
    {
        left = left & right;
        return left;
    }

    static inline auto default_resource_flags_compare_function(
        ice::ResourceFlags expected,
        ice::ResourceFlags current,
        ice::ResourceFlags /*selected*/
    ) noexcept -> ice::u32
    {
        // The actual meaning of this value does not matter. We only want a single value that we can use to
        //  return a value higher whenever the match is better.
        ice::u32 const max_value = static_cast<ice::u32>(ResourceFlags::Quality_Lowest);

        ice::i32 const expected_value = static_cast<ice::i32>(expected & ResourceFlags::Mask_Quality);
        ice::i32 const current_value = static_cast<ice::i32>(current & ResourceFlags::Mask_Quality);

        // We subtract `expected` and `current` values, and get the absolute result of it.
        //  We then subtract it again from our max value. The closer we get to our match the higher the value gets.
        //  The priority for that match also goes up.
        return max_value - ice::abs(expected_value - current_value);
    }

} // namespace ice
