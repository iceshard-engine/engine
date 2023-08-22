/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task_types.hxx>

namespace ice
{

    //! \brief Enables an enum type to be accepted as a task enumeration.
    template<typename Enum>
    static constexpr bool Constant_IsTaskFlagsEnumeration = false;

    //! \brief Base type required for an Enum to be a valid TaskFlag enumeration.
    using TaskFlagBaseType = ice::u32;

    //! \brief Enumeration values used in the engine explicitly.
    static constexpr TaskFlagBaseType Constant_TaskFlagLongValue = 0x1;
    static constexpr TaskFlagBaseType Constant_TaskFlagLowPrioValue = 0x2;
    static constexpr TaskFlagBaseType Constant_TaskFlagNormalPrioValue = 0x4;
    static constexpr TaskFlagBaseType Constant_TaskFlagHighPrioValue = 0x8;

    template<typename Enum>
    concept TaskFlagType = ice::FlagType<Enum> && ice::FlagAllValue<Enum> // Ensure it's a IceShard flag enabled type
        // Ensure the flag type is manually enabled and matches the expected base type
        && Constant_IsTaskFlagsEnumeration<Enum> && std::is_same_v<ice::TaskFlagBaseType, std::underlying_type_t<Enum>>
        // Ensure the specified names are part of the enum.
        && requires(Enum t) {
            { Enum::Long } -> std::convertible_to<Enum>;
            { Enum::PrioLow } -> std::convertible_to<Enum>;
            { Enum::PrioNormal } -> std::convertible_to<Enum>;
            { Enum::PrioHigh } -> std::convertible_to<Enum>;
        }
        // Check if the flag is part of 'All' and does it have the expected value
        && static_cast<ice::TaskFlagBaseType>(Enum::All & Enum::Long) == Constant_TaskFlagLongValue
        && static_cast<ice::TaskFlagBaseType>(Enum::All & Enum::PrioLow) == Constant_TaskFlagLowPrioValue
        && static_cast<ice::TaskFlagBaseType>(Enum::All & Enum::PrioNormal) == Constant_TaskFlagNormalPrioValue
        && static_cast<ice::TaskFlagBaseType>(Enum::All & Enum::PrioHigh) == Constant_TaskFlagHighPrioValue;

    //! \brief
    struct TaskFlags
    {
        ice::TaskFlagBaseType value = Constant_TaskFlagNormalPrioValue;

        constexpr explicit TaskFlags() noexcept = default;

        template<ice::TaskFlagType Flags>
        constexpr explicit TaskFlags(Flags flags) noexcept
            : value{ static_cast<ice::TaskFlagBaseType>(flags) }
        {
        }

        template<ice::TaskFlagType Flags>
        constexpr auto operator=(Flags flags) noexcept -> TaskFlags&
        {
            value = static_cast<ice::TaskFlagBaseType>(flags);
            return *this;
        }

        template<ice::TaskFlagType Flags>
        constexpr operator Flags() const noexcept
        {
            return static_cast<Flags>(value);
        }
    };

} // namespace ice
