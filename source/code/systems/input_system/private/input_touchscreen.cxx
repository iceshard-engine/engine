/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "input_devices.hxx"
#include "input_state_helpers.hxx"

#include <ice/input/input_mouse.hxx>
#include <ice/input/input_touchscreen.hxx>
#include <ice/container/array.hxx>

namespace ice::input
{

    class TouchScreenDevice : public InputDevice
    {
    public:
        TouchScreenDevice(
            ice::Allocator& alloc,
            ice::input::DeviceHandle device
        ) noexcept;

        auto handle() const noexcept -> ice::input::DeviceHandle override
        {
            return _device;
        }

        void on_tick(ice::Timer const& timer) noexcept override;
        void on_event(ice::input::DeviceEvent event) noexcept override;
        void on_publish(ice::Array<ice::input::InputEvent>& events_out) noexcept override;

    private:
        ice::input::DeviceHandle _device;
        ice::Array<detail::ControlState> _controls;

        ice::vec2f _screen_size;
        // Holds x, y and age in float ms passed.
        ice::vec2f _screen_points[256]; // Support up to five "tentacles"...

        ice::vec2f _last_touch_pos[5];
    };

    TouchScreenDevice::TouchScreenDevice(
        ice::Allocator& alloc,
        ice::input::DeviceHandle device
    ) noexcept
        : _device{ device }
        , _controls{ alloc }
    {
        ice::array::resize(_controls, 0);
        for (detail::ControlState& control : _controls)
        {
            control.id = InputID::Invalid;
            control.tick = 0;
            control.value.button.value_i32 = 0;
        }
    }

    void TouchScreenDevice::on_tick(ice::Timer const& timer) noexcept
    {
        for (detail::ControlState& control : _controls)
        {
            detail::handle_value_button_hold_and_repeat(control);
        }
    }

    void TouchScreenDevice::on_event(ice::input::DeviceEvent event) noexcept
    {
        InputID input = InputID::Invalid;

        ice::u8 last_touch_id = 0;
        switch (event.message)
        {
        case DeviceMessage::TouchScreenSizeX:
            _screen_size.x = event.payload_data.val_f32;
            break;
        case DeviceMessage::TouchScreenSizeY:
            _screen_size.y = event.payload_data.val_f32;
            break;

        case DeviceMessage::TouchEvent:
            last_touch_id = event.payload_data.val_i8;
            break;
        case DeviceMessage::TouchPosition:
            _last_touch_pos[last_touch_id].x = event.payload_data.val_i16x2.x;
            _last_touch_pos[last_touch_id].y = event.payload_data.val_i16x2.y;
            break;
        case DeviceMessage::TouchPositionX:
            _last_touch_pos[last_touch_id].x = event.payload_data.val_f32;
            break;
        case DeviceMessage::TouchPositionY:
            _last_touch_pos[last_touch_id].y = event.payload_data.val_f32;
            break;
        case DeviceMessage::TouchEnd:
            ICE_ASSERT_CORE(last_touch_id == event.payload_data.val_i8);
            break;
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
                detail::handle_value_button_up(control);
            }

            _controls[control_index] = control;
        }
    }

    void TouchScreenDevice::on_publish(ice::Array<ice::input::InputEvent>& events_out) noexcept
    {
        InputEvent event{
            .device = _device
        };

        event.identifier = input_identifier(DeviceType::TouchScreen, TouchInput::TouchPosX);
        event.value.axis.value_i32 = ice::f32(_last_touch_pos[0].x);
        event.value_type = InputValueType::AxisInt;
        ice::array::push_back(events_out, event);

        event.identifier = input_identifier(DeviceType::TouchScreen, TouchInput::TouchPosY);
        event.value.axis.value_i32 = ice::f32(_last_touch_pos[0].y);
        event.value_type = InputValueType::AxisInt;
        ice::array::push_back(events_out, event);

        // event.identifier = input_identifier(DeviceType::Mouse, MouseInput::PositionY);
        // event.value.axis.value_i32 = _position[1];
        // event.value_type = InputValueType::AxisInt;
        // ice::array::push_back(events_out, event);

        // if (_position_relative[0] != 0)
        // {
        //     event.identifier = input_identifier(DeviceType::Mouse, MouseInput::PositionXRelative);
        //     event.value.axis.value_i32 = _position_relative[0];
        //     event.value_type = InputValueType::AxisInt;
        //     ice::array::push_back(events_out, event);
        // }
        // if (_position_relative[1] != 0)
        // {
        //     event.identifier = input_identifier(DeviceType::Mouse, MouseInput::PositionYRelative);
        //     event.value.axis.value_i32 = _position_relative[1];
        //     event.value_type = InputValueType::AxisInt;
        //     ice::array::push_back(events_out, event);
        // }

        // if (_wheel != 0)
        // {
        //     event.identifier = input_identifier(DeviceType::Mouse, MouseInput::Wheel);
        //     event.value.axis.value_i32 = _wheel;
        //     ice::array::push_back(events_out, event);
        // }

        // for (detail::ControlState& control : _controls)
        // {
        //     if (detail::prepared_input_event(control, event))
        //     {
        //         ice::array::push_back(events_out, event);
        //     }
        // }
    }

    auto create_touchscreen_device(
        ice::Allocator& alloc,
        ice::input::DeviceHandle device
    ) noexcept -> ice::input::InputDevice*
    {
        return alloc.create<TouchScreenDevice>(alloc, device);
    }

} // namespace ice::input
