/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/input/device_handle.hxx>
#include <ice/math.hxx>
#include <ice/mem.hxx>

namespace ice::input
{

    enum class DeviceMessage : ice::u8
    {
        Invalid = 0x0,

        DeviceConnected,
        DeviceDisconnected,

        MousePositionX,
        MousePositionY,
        MouseButtonDown,
        MouseButtonUp,
        MouseWheel,

        KeyboardButtonDown,
        KeyboardButtonUp,
        KeyboardModifierDown,
        KeyboardModifierUp,

        GamepadButtonDown,
        GamepadButtonUp,
        GamepadTriggerLeft,
        GamepadTriggerRight,
        GamepadAxisLeftX,
        GamepadAxisLeftY,
        GamepadAxisRightX,
        GamepadAxisRightY,

        TouchScreenSizeX,
        TouchScreenSizeY,

        TouchStart,
        TouchPositionX,
        TouchPositionY,
        TouchEnd,

        TouchGestureStart,
        TouchGestureNext,
        TouchGestureTime, // Returned for each event coming from the OS
        TouchGestureEnd,

        Reserved
    };

    enum class DevicePayloadType : ice::u8
    {
        Invalid,
        Int8,
        Int16,
        Int16x2,
        Int32,
        Float32,
        Enum,
    };

    union DeviceEventData
    {
        ice::i8 val_i8;
        ice::i16 val_i16;
        ice::math::vec<2, ice::i16> val_i16x2;
        ice::i32 val_i32;
        ice::f32 val_f32;
    };

    struct DeviceEvent
    {
        ice::input::DeviceHandle device;
        ice::input::DeviceMessage message;
        ice::input::DevicePayloadType payload_type;
        ice::u8 _reserved;
        ice::input::DeviceEventData payload_data;
    };

    static_assert(
        ice::size_of<DeviceEvent> == 8_B,
        "Unexpected size for 'DeviceEvent' type"
    );

    template<typename>
    constexpr DevicePayloadType Constant_PayloadType = DevicePayloadType::Invalid;
    template<>
    constexpr inline DevicePayloadType Constant_PayloadType<ice::i8> = DevicePayloadType::Int8;
    template<>
    constexpr inline DevicePayloadType Constant_PayloadType<ice::i16> = DevicePayloadType::Int16;
    template<>
    constexpr inline DevicePayloadType Constant_PayloadType<ice::math::vec<2, ice::i16>> = DevicePayloadType::Int16x2;
    template<>
    constexpr inline DevicePayloadType Constant_PayloadType<ice::i32> = DevicePayloadType::Int32;
    template<>
    constexpr inline DevicePayloadType Constant_PayloadType<ice::f32> = DevicePayloadType::Float32;
    template<typename T> requires std::is_enum_v<T>
    constexpr inline DevicePayloadType Constant_PayloadType<T> = DevicePayloadType::Enum;

} // ice::input
