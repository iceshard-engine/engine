#pragma once
#include <ice/input/device_handle.hxx>

namespace ice::input
{

    enum class DeviceMessage : ice::u8
    {
        Invalid = 0x0,

        DeviceConnected,
        DeviceDisconnected,
        MousePosition,
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
        GamepadAxisLeft,
        GamepadAxisLeftX,
        GamepadAxisLeftY,
        GamepadAxisRight,
        GamepadAxisRightX,
        GamepadAxisRightY,

        Reserved
    };

    struct DeviceEventPayload
    {
        ice::u8 type;
        ice::u8 size : 4;
        ice::u8 count : 4;
    };

    struct DeviceEvent
    {
        ice::input::DeviceHandle device;
        ice::input::DeviceMessage message;
        ice::input::DeviceEventPayload payload;
    };

    static_assert(sizeof(DeviceEvent) == 4, "Unexpected size for 'DeviceEvent' type");



    namespace detail
    {

        template<typename T>
        constexpr DeviceEventPayload payload_info{ };

        template<>
        constexpr DeviceEventPayload payload_info<bool>
        {
            .type = 0x1,
            .size = 1,
            .count = 0
        };

        template<>
        constexpr DeviceEventPayload payload_info<ice::i8>
        {
            .type = 0x2,
            .size = 1,
            .count = 0
        };

        template<>
        constexpr DeviceEventPayload payload_info<ice::i16>
        {
            .type = 0x3,
            .size = 2,
            .count = 0
        };

        template<>
        constexpr DeviceEventPayload payload_info<ice::u16>
        {
            .type = 0x4,
            .size = 2,
            .count = 0
        };

        template<>
        constexpr DeviceEventPayload payload_info<ice::i32>
        {
            .type = 0x5,
            .size = 4,
            .count = 0
        };

        template<>
        constexpr DeviceEventPayload payload_info<ice::f32>
        {
            .type = 0x6,
            .size = 4,
            .count = 0
        };

    } // namespace detail

} // ice::input
