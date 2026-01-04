/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "input_devices.hxx"
#include "input_state_helpers.hxx"

#include <ice/input/input_mouse.hxx>
#include <ice/input/input_touchscreen.hxx>
#include <ice/container/array.hxx>

namespace ice::input
{

    //! \brief Maximum number of pointers supported on a touchscreen device.
    static constexpr ice::u32 Constant_MaxPointerCount = 6;

    static constexpr ice::input::detail::ControlConfig Constant_TouchControlConfig {
        .button_click_threshold = Constant_TouchScreenClickThreshold,
        .button_hold_threshold = Constant_TouchScreenHoldThreshold
    };

    class TouchScreenDevice : public InputDevice
    {
    public:
        TouchScreenDevice(
            ice::Allocator& alloc,
            ice::input::DeviceHandle device
        ) noexcept;

        auto max_count() const noexcept -> ice::u32 override
        {
            return Constant_MaxPointerCount;
        }

        auto count() const noexcept -> ice::u32 override
        {
            return _pointer_count;
        }

        auto handle(ice::u32 index) const noexcept -> ice::input::DeviceHandle override
        {
            ICE_ASSERT_CORE(index >= 0 && index < Constant_MaxPointerCount);
            return _pointers[index];
        }

        void on_tick(ice::Timer const& timer) noexcept override;
        void on_event(ice::input::DeviceEvent event) noexcept override;
        void on_publish(ice::Array<ice::input::InputEvent>& events_out) noexcept override;

    private:
        ice::u32 _pointer_count;
        ice::input::DeviceHandle _pointers[Constant_MaxPointerCount];
        ice::vec2f _positions[Constant_MaxPointerCount];


        ice::Array<detail::ControlState> _controls;

        ice::vec2f _screen_size;
        // Holds x, y and age in float ms passed.
        // ice::vec2f _screen_points[256]; // Support up to five "tentacles"...

        bool _changed_pointer_count;
        // ice::vec2f _last_touch_pos[5];
    };

    TouchScreenDevice::TouchScreenDevice(
        ice::Allocator& alloc,
        ice::input::DeviceHandle device
    ) noexcept
        : _pointer_count{ 0 }
        , _pointers{ }
        , _controls{ alloc }
        , _changed_pointer_count{ false }
    {
        for (auto& pointer : _pointers)
        {
            pointer = ice::input::DeviceHandle::Invalid;
        }

        ice::array::resize(_controls, Constant_MaxPointerCount + 1);
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
            detail::handle_value_button_hold_and_repeat(control, Constant_TouchControlConfig);
        }
    }

    void TouchScreenDevice::on_event(ice::input::DeviceEvent event) noexcept
    {
        InputID input = InputID::Invalid;

        ice::u8 const pointer_index = static_cast<ice::u8>(ice::input::make_device(event.device).index);
        switch (event.message)
        {
        case DeviceMessage::TouchScreenSizeX:
            _screen_size.x = event.payload_data.val_f32;
            break;
        case DeviceMessage::TouchScreenSizeY:
            _screen_size.y = event.payload_data.val_f32;
            break;

        case DeviceMessage::TouchStart:
            _changed_pointer_count = true;
            ICE_ASSERT_CORE(_pointers[pointer_index] == DeviceHandle::Invalid);
            _pointers[pointer_index] = event.device;
            _pointer_count += 1;
            input = input_identifier(DeviceType::TouchPointer, TouchInput::VirtualButton);
            break;
        case DeviceMessage::TouchEnd:
            _changed_pointer_count = true;
            _pointer_count -= 1;
            ICE_ASSERT_CORE(_pointers[pointer_index] != DeviceHandle::Invalid);
            _pointers[pointer_index] = DeviceHandle::Invalid;
            input = input_identifier(DeviceType::TouchPointer, TouchInput::VirtualButton);
            break;
        case DeviceMessage::TouchPositionX:
            _positions[pointer_index].x = event.payload_data.val_f32;
            break;
        case DeviceMessage::TouchPositionY:
            _positions[pointer_index].y = event.payload_data.val_f32;
            break;
        default:
            return;
        }

        if (input != InputID::Invalid)
        {
            ice::u32 const control_index = input_identifier_value(input) + pointer_index;
            ICE_ASSERT_CORE(control_index < ice::array::count(_controls));

            detail::ControlState control = _controls[control_index];
            control.id = input;

            if (event.message == DeviceMessage::TouchStart)
            {
                detail::handle_value_button_down(control);
            }
            else if (event.message == DeviceMessage::TouchEnd)
            {
                detail::handle_value_button_up(control, Constant_TouchControlConfig);
            }

            _controls[control_index] = control;
        }
    }

    void TouchScreenDevice::on_publish(ice::Array<ice::input::InputEvent>& events_out) noexcept
    {
        InputEvent event{ };

        if (_changed_pointer_count)
        {
            _changed_pointer_count = false;
            event.device = make_device_handle(DeviceType::TouchScreen, DeviceIndex{});
            event.identifier = input_identifier(DeviceType::TouchScreen, TouchInput::TouchPointerCount);
            event.value_type = InputValueType::AxisInt;
            event.value.axis.value_i32 = _pointer_count;
            ice::array::push_back(events_out, event);
        }

        event.value_type = InputValueType::AxisFloat;

        ice::u32 remaining = _pointer_count;
        for (ice::u32 idx = 0; idx < max_count() && remaining > 0; ++idx)
        {
            if (_pointers[idx] == DeviceHandle::Invalid) continue;

            // ice::u32 const data_idx = ice::u8(make_device(_pointers[idx]).index);
            event.device = _pointers[idx];
            event.identifier = input_identifier(DeviceType::TouchScreen, TouchInput::TouchPosX);
            event.value.axis.value_f32 = (_positions[idx].x / _screen_size.x) - 0.5f;
            ice::array::push_back(events_out, event);

            event.identifier = input_identifier(DeviceType::TouchScreen, TouchInput::TouchPosY);
            event.value.axis.value_f32 = (_positions[idx].y / _screen_size.y) - 0.5f;
            ice::array::push_back(events_out, event);
            remaining -= 1;
        }

        ice::i32 control_index = -1;
        for (detail::ControlState& control : _controls)
        {
            event.device = make_device_handle(DeviceType::TouchPointer, DeviceIndex(control_index++));
            if (detail::prepared_input_event(control, event))
            {
                ice::array::push_back(events_out, event);
            }
        }
    }

    auto create_touchscreen_device(
        ice::Allocator& alloc,
        ice::input::DeviceHandle device
    ) noexcept -> ice::input::InputDevice*
    {
        return alloc.create<TouchScreenDevice>(alloc, device);
    }

} // namespace ice::input
