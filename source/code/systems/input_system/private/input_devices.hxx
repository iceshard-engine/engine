#pragma once
#include <ice/input/input_device.hxx>

namespace ice::input
{

    auto create_mouse_device(
        ice::Allocator& alloc,
        ice::input::DeviceHandle device
    ) noexcept -> ice::input::InputDevice*;

    auto create_keyboard_device(
        ice::Allocator& alloc,
        ice::input::DeviceHandle device
    ) noexcept -> ice::input::InputDevice*;

    auto create_controller_device(
        ice::Allocator& alloc,
        ice::input::DeviceHandle device
    ) noexcept -> ice::input::InputDevice*;

} // namespace ice::input
