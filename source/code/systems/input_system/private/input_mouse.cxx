/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "input_devices.hxx"
#include "input_state_helpers.hxx"

#include <ice/input/input_mouse.hxx>
#include <ice/container/array.hxx>

namespace ice::input
{

    static constexpr ice::u32 mouse_button_num = static_cast<ice::u32>(MouseInput::ButtonCustom1);

    class MouseDevice : public InputDevice
    {
    public:
        MouseDevice(
            ice::Allocator& alloc,
            ice::input::DeviceHandle device
        ) noexcept;

        auto handle(ice::u32) const noexcept -> ice::input::DeviceHandle override
        {
            return _device;
        }

        void on_tick(ice::Timer const& timer) noexcept override;
        void on_event(ice::input::DeviceEvent event) noexcept override;
        void on_publish(ice::Array<ice::input::InputEvent>& events_out) noexcept override;

    private:
        ice::input::DeviceHandle _device;
        ice::Array<detail::ControlState> _controls;

        ice::i32 _position[2]{ 0, 0 };
        ice::i32 _position_relative[2]{ 0, 0 };
        ice::i32 _wheel = 0;
    };

    MouseDevice::MouseDevice(
        ice::Allocator& alloc,
        ice::input::DeviceHandle device
    ) noexcept
        : _device{ device }
        , _controls{ alloc }
    {
        ice::array::resize(_controls, mouse_button_num + 5);
        for (detail::ControlState& control : _controls)
        {
            control.id = InputID::Invalid;
            control.tick = 0;
            control.value.button.value_i32 = 0;
        }
    }

    void MouseDevice::on_tick(ice::Timer const& timer) noexcept
    {
        _wheel = 0;
        _position_relative[0] = 0;
        _position_relative[1] = 0;

        for (detail::ControlState& control : _controls)
        {
            detail::handle_value_button_hold_and_repeat(control, detail::Constant_DefaultControlConfig);
        }
    }

    void MouseDevice::on_event(ice::input::DeviceEvent event) noexcept
    {
        InputID input = InputID::Invalid;

        switch (event.message)
        {
        case DeviceMessage::MouseButtonDown:
        case DeviceMessage::MouseButtonUp:
            input = input_identifier(DeviceType::Mouse, detail::event_data<MouseInput>(event));
            break;
        case DeviceMessage::MousePositionX:
            _position_relative[0] = detail::event_data<ice::i32>(event) - _position[0];
            _position[0] = detail::event_data<ice::i32>(event);
            break;
        case DeviceMessage::MousePositionY:
            _position_relative[1] = detail::event_data<ice::i32>(event) - _position[1];
            _position[1] = detail::event_data<ice::i32>(event);
            break;
        case DeviceMessage::MouseWheel:
            _wheel = detail::event_data<ice::i32>(event);
            return;
        default:
            return;
        }

        if (input != InputID::Invalid)
        {
            ice::u32 const control_index = input_identifier_value(input);
            ICE_ASSERT_CORE(control_index < ice::array::count(_controls));

            detail::ControlState control = _controls[control_index];
            control.id = input;

            if (event.message == DeviceMessage::MouseButtonDown)
            {
                detail::handle_value_button_down(control);
            }
            else if (event.message == DeviceMessage::MouseButtonUp)
            {
                detail::handle_value_button_up(control, detail::Constant_DefaultControlConfig);
            }

            _controls[control_index] = control;
        }
    }

    void MouseDevice::on_publish(ice::Array<ice::input::InputEvent>& events_out) noexcept
    {
        InputEvent event{
            .device = _device
        };

        event.identifier = input_identifier(DeviceType::Mouse, MouseInput::PositionX);
        event.axis_idx = 0;
        event.value.axis.value_i32 = _position[0];
        event.value_type = InputValueType::AxisInt;
        ice::array::push_back(events_out, event);

        event.identifier = input_identifier(DeviceType::Mouse, MouseInput::PositionY);
        event.axis_idx = 1;
        event.value.axis.value_i32 = _position[1];
        event.value_type = InputValueType::AxisInt;
        ice::array::push_back(events_out, event);

        if (_position_relative[0] != 0)
        {
            event.identifier = input_identifier(DeviceType::Mouse, MouseInput::PositionXRelative);
            event.axis_idx = 0;
            event.value.axis.value_i32 = _position_relative[0];
            event.value_type = InputValueType::AxisInt;
            ice::array::push_back(events_out, event);
        }
        if (_position_relative[1] != 0)
        {
            event.identifier = input_identifier(DeviceType::Mouse, MouseInput::PositionYRelative);
            event.axis_idx = 1;
            event.value.axis.value_i32 = _position_relative[1];
            event.value_type = InputValueType::AxisInt;
            ice::array::push_back(events_out, event);
        }

        if (_wheel != 0)
        {
            event.identifier = input_identifier(DeviceType::Mouse, MouseInput::Wheel);
            event.value.axis.value_i32 = _wheel;
            ice::array::push_back(events_out, event);
        }

        for (detail::ControlState& control : _controls)
        {
            if (detail::prepared_input_event(control, event))
            {
                ice::array::push_back(events_out, event);
            }
        }
    }

    auto create_mouse_device(
        ice::Allocator& alloc,
        ice::input::DeviceHandle device
    ) noexcept -> ice::input::InputDevice*
    {
        return alloc.create<MouseDevice>(alloc, device);
    }

} // namespace ice::input
