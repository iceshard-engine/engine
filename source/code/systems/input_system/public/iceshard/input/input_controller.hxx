#pragma once
#include <core/allocator.hxx>
#include <iceshard/input/input_event.hxx>
#include <iceshard/input/device/input_device.hxx>

namespace iceshard::input
{

    enum class ControllerInput : uint16_t
    {
        Unknown = 0x0,

        ButtonA,
        ButtonB,
        ButtonX,
        ButtonY,
        ButtonUp,
        ButtonDown,
        ButtonLeft,
        ButtonRight,
        ButtonLeftShoulder,
        ButtonRightShoulder,

        LeftTrigger,
        RightTrigger,

        LeftAxisX,
        LeftAxisY,
        RightAxisX,
        RightAxisY,
    };

    struct DeviceState;

    auto default_controller_state_factory(
        core::allocator& alloc,
        DeviceHandle device_type
    ) noexcept -> DeviceState*;

    inline constexpr auto create_inputid(DeviceType, ControllerInput value) noexcept -> InputID
    {
        return create_inputid(DeviceType::Controller, static_cast<uint16_t>(value));
    }

} // namespace iceshard::input
