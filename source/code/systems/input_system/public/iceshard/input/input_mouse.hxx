#pragma once
#include <core/allocator.hxx>
#include <iceshard/input/input_event.hxx>
#include <iceshard/input/device/input_device.hxx>

namespace iceshard::input
{

    enum class MouseInput : uint8_t
    {
        Unknown

        , ButtonLeft
        , ButtonRight
        , ButtonMiddle

        , ButtonCustom0
        , ButtonCustom1

        , PositionX
        , PositionY
        , PositionXRelative
        , PositionYRelative
        , Wheel
    };

    struct DeviceState;

    auto default_mouse_state_factory(
        core::allocator& alloc,
        DeviceHandle device_type
    ) noexcept -> DeviceState*;

    inline constexpr auto create_inputid(DeviceType, MouseInput value) noexcept -> InputID
    {
        return create_inputid(DeviceType::Mouse, static_cast<uint16_t>(value));
    }

} // namespace iceshard::input
