#pragma once
#include <iceshard/input/device/input_device_data.hxx>

namespace iceshard::input
{

    enum class DeviceHandle : uint8_t;

    enum class DeviceInputType : uint8_t
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
    };

    struct DeviceInputMessage
    {
        DeviceHandle device;
        DeviceInputType input_type;
        DeviceInputData input_data;
    };

    static_assert(sizeof(DeviceInputMessage) == 4);

} // namespace iceshard::input
