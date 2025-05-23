/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/input/device_handle.hxx>
#include <ice/shard.hxx>
#include <ice/hash.hxx>
#include <ice/math.hxx>

namespace ice::input
{

    enum class InputID : ice::u16
    {
        Invalid = 0x0,
    };

    constexpr inline auto input_identifier(
        ice::input::DeviceType type,
        ice::u16 value
    ) noexcept -> ice::input::InputID;

    template<typename T>
    constexpr inline auto input_identifier(
        ice::input::DeviceType type,
        T value,
        ice::u16 base_value = 0
    ) noexcept -> ice::input::InputID;


    enum class InputValueType : ice::u8
    {
        Invalid,
        Button,
        Trigger,
        AxisInt,
        AxisFloat,
    };


    union InputValue
    {
        union
        {

            ice::f32 value_f32;
            ice::i32 value_i32;

        } axis;

        union
        {

            struct
            {

                bool pressed : 1;
                bool hold : 1;
                bool released : 1;
                bool clicked : 1;
                ice::u16 repeat;

            } state;

            ice::i32 value_i32;

        } button;

        union
        {

            ice::f32 value_f32;

        } trigger;
    };

    static_assert(sizeof(InputValue) == 4);


    struct InputEvent
    {
        ice::input::InputID identifier;
        ice::input::DeviceHandle device;

        bool reserved : 2;
        bool initial_event : 1;
        ice::u8 axis_idx : 2;
        ice::input::InputValueType value_type : 3;

        ice::input::InputValue value;
    };

    static_assert(sizeof(InputEvent) == 8);


    constexpr inline auto input_identifier(
        ice::input::DeviceType type,
        ice::u16 value
    ) noexcept -> ice::input::InputID
    {
        return static_cast<ice::input::InputID>((static_cast<ice::u16>(type) << 12) | value);
    }

    template<typename T>
    constexpr inline auto input_identifier(
        ice::input::DeviceType type,
        T value,
        ice::u16 base_value
    ) noexcept -> ice::input::InputID
    {
        using UnderlyingType = std::underlying_type_t<T>;

        if constexpr (sizeof(UnderlyingType) <= 2)
        {
            base_value += static_cast<ice::u16>(value);
            return input_identifier(type, base_value);
        }
        else
        {
            return InputID::Invalid;
        }
    }

    constexpr inline auto input_identifier_device(
        ice::input::InputID input
    ) noexcept -> ice::input::DeviceType
    {
        constexpr ice::u16 identifier_device_mask = 0xf000;
        return static_cast<DeviceType>((static_cast<ice::u16>(input) & identifier_device_mask) >> 12);
    }

    constexpr inline auto input_identifier_value(
        ice::input::InputID input
    ) noexcept -> ice::u16
    {
        constexpr ice::u16 identifier_value_mask = 0x0fff;
        return static_cast<ice::u16>(input) & identifier_value_mask;
    }

} // namespace ice::input

template<>
constexpr inline auto ice::hash<ice::input::InputID>(ice::input::InputID value) noexcept -> ice::u64
{
    return static_cast<ice::u64>(value);
}

template<>
constexpr inline ice::ShardPayloadID ice::Constant_ShardPayloadID<ice::input::InputEvent> = ice::shard_payloadid("ice::input::InputEvent");
