#include "input_devices.hxx"
#include "input_state_helpers.hxx"

#include <ice/input/input_controller.hxx>
#include <ice/pod/array.hxx>

namespace ice::input
{

    static constexpr ice::u32 controller_button_num = static_cast<ice::u32>(ControllerInput::RightTrigger);

    class ControllerDevice : public InputDevice
    {
    public:
        ControllerDevice(
            ice::Allocator& alloc,
            ice::input::DeviceHandle device
        ) noexcept;

        void on_tick(
            ice::Timer const& timer
        ) noexcept override;

        void on_event(
            ice::input::DeviceEvent event,
            ice::Data payload
        ) noexcept override;

        void on_publish(
            ice::pod::Array<ice::input::InputEvent>& events_out
        ) noexcept override;

    private:
        ice::input::DeviceHandle _device;
        ice::pod::Array<detail::ControlState> _controls;

        ice::f32 _left_axis[2]{ 0, 0 };
        ice::f32 _right_axis[2]{ 0, 0 };
        ice::f32 _triggers[2]{ 0, 0 };

        bool _axis_reset_event[6]{ true, true, true, true, true, true };
    };

    ControllerDevice::ControllerDevice(
        ice::Allocator& alloc,
        ice::input::DeviceHandle device
    ) noexcept
        : _device{ device }
        , _controls{ alloc }
    {
        ice::pod::array::resize(_controls, controller_button_num + 5);
    }

    void ControllerDevice::on_tick(ice::Timer const& timer) noexcept
    {
        //auto reset_in_deadzone = [](ice::f32& val) noexcept
        //{
        //    if (val <= 0.15f && val >= -0.15f)
        //    {
        //        val = 0.0f;
        //    }
        //};

        //reset_in_deadzone(_left_axis[0]);
        //reset_in_deadzone(_left_axis[1]);
        //reset_in_deadzone(_right_axis[0]);
        //reset_in_deadzone(_right_axis[1]);
        //reset_in_deadzone(_triggers[0]);
        //reset_in_deadzone(_triggers[1]);

        for (detail::ControlState& control : _controls)
        {
            detail::handle_value_button_hold(control);
        }
    }

    void ControllerDevice::on_event(
        ice::input::DeviceEvent event,
        ice::Data payload
    ) noexcept
    {
        InputID input = InputID::Invalid;

        switch (event.message)
        {
        case DeviceMessage::GamepadButtonDown:
        case DeviceMessage::GamepadButtonUp:
            input = input_identifier(DeviceType::Controller, detail::read_one<ControllerInput>(event, payload));
            break;
        case DeviceMessage::GamepadTriggerLeft:
            _triggers[0] = detail::read_one<ice::f32>(event, payload);
            return;
        case DeviceMessage::GamepadTriggerRight:
            _triggers[1] = detail::read_one<ice::f32>(event, payload);
            return;
        case DeviceMessage::GamepadAxisLeft:
        {
            ice::Span<ice::f32 const> const axis_value = detail::read<ice::f32>(event, payload);
            _left_axis[0] = axis_value[0];
            _left_axis[1] = axis_value[1];
            return;
        }
        case DeviceMessage::GamepadAxisLeftX:
            _left_axis[0] = detail::read_one<ice::f32>(event, payload);
            return;
        case DeviceMessage::GamepadAxisLeftY:
            _left_axis[1] = detail::read_one<ice::f32>(event, payload);
            return;
        case DeviceMessage::GamepadAxisRight:
        {
            ice::Span<ice::f32 const> const axis_value = detail::read<ice::f32>(event, payload);
            _right_axis[0] = axis_value[0];
            _right_axis[1] = axis_value[1];
            return;
        }
        case DeviceMessage::GamepadAxisRightX:
            _right_axis[0] = detail::read_one<ice::f32>(event, payload);
            return;
        case DeviceMessage::GamepadAxisRightY:
            _right_axis[1] = detail::read_one<ice::f32>(event, payload);
            return;
        default:
            return;
        }

        if (input != InputID::Invalid)
        {
            ice::i32 const control_index = input_identifier_value(input);
            // #todo assert control_index < ice::pod::array::size(_controls)

            detail::ControlState control = _controls[control_index];
            control.id = input;

            if (event.message == DeviceMessage::GamepadButtonDown)
            {
                detail::handle_value_button_down(control);
            }
            else if (event.message == DeviceMessage::GamepadButtonUp)
            {
                detail::handle_value_button_up(control);
            }

            _controls[control_index] = control;
        }
    }

    void ControllerDevice::on_publish(
        ice::pod::Array<ice::input::InputEvent>& events_out
    ) noexcept
    {
        InputEvent event{
            .device = _device
        };

        auto publish_axis_value = [&event, &events_out](ControllerInput input, ice::f32 value, bool& reset) noexcept
        {
            if (value >= 0.25 || value <= -0.25f)
            {
                reset = false;
                event.identifier = input_identifier(DeviceType::Controller, input);
                event.value.axis.value_f32 = value;
                ice::pod::array::push_back(events_out, event);
            }
            else if (reset == false)
            {
                reset = true;
                event.identifier = input_identifier(DeviceType::Controller, input);
                event.value.axis.value_f32 = 0.0f;
                ice::pod::array::push_back(events_out, event);
            }
        };

        publish_axis_value(ControllerInput::LeftAxisX, _left_axis[0], _axis_reset_event[0]);
        publish_axis_value(ControllerInput::LeftAxisY, _left_axis[1], _axis_reset_event[1]);
        publish_axis_value(ControllerInput::RightAxisX, _right_axis[0], _axis_reset_event[2]);
        publish_axis_value(ControllerInput::RightAxisY, _right_axis[1], _axis_reset_event[3]);
        publish_axis_value(ControllerInput::LeftTrigger, _triggers[0], _axis_reset_event[4]);
        publish_axis_value(ControllerInput::RightTrigger, _triggers[1], _axis_reset_event[5]);

        for (detail::ControlState& control : _controls)
        {
            if (detail::prepared_input_event(control, event))
            {
                ice::pod::array::push_back(events_out, event);
            }
        }
    }

    auto create_controller_device(
        ice::Allocator& alloc,
        ice::input::DeviceHandle device
    ) noexcept -> ice::input::InputDevice*
    {
        return alloc.make<ControllerDevice>(alloc, device);
    }

} // ice::input
