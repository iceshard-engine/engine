#include "input_devices.hxx"
#include "input_state_helpers.hxx"

#include <ice/input/input_mouse.hxx>
#include <ice/pod/array.hxx>

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

        auto handle() const noexcept -> ice::input::DeviceHandle override
        {
            return _device;
        }

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
        ice::pod::array::resize(_controls, mouse_button_num + 5);
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
            detail::handle_value_button_hold_and_repeat(control);
        }
    }

    void MouseDevice::on_event(
        ice::input::DeviceEvent event,
        ice::Data payload
    ) noexcept
    {
        InputID input = InputID::Invalid;

        switch (event.message)
        {
        case DeviceMessage::MouseButtonDown:
        case DeviceMessage::MouseButtonUp:
            input = input_identifier(DeviceType::Mouse, detail::read_one<MouseInput>(event, payload));
            break;
        case DeviceMessage::MousePosition:
        {
            ice::Span<ice::i32 const> const pos = detail::read<ice::i32>(event, payload);

            _position_relative[0] = pos[0] - _position[0];
            _position_relative[1] = pos[1] - _position[1];

            _position[0] = pos[0];
            _position[1] = pos[1];
            return;
        }
        case DeviceMessage::MouseWheel:
            _wheel = detail::read_one<ice::i32>(event, payload);
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

    void MouseDevice::on_publish(
        ice::pod::Array<ice::input::InputEvent>& events_out
    ) noexcept
    {
        InputEvent event{
            .device = _device
        };

        event.identifier = input_identifier(DeviceType::Mouse, MouseInput::PositionX);
        event.value.axis.value_i32 = _position[0];
        ice::pod::array::push_back(events_out, event);

        event.identifier = input_identifier(DeviceType::Mouse, MouseInput::PositionY);
        event.value.axis.value_i32 = _position[1];
        ice::pod::array::push_back(events_out, event);

        if (_position_relative[0] != 0)
        {
            event.identifier = input_identifier(DeviceType::Mouse, MouseInput::PositionXRelative);
            event.value.axis.value_i32 = _position_relative[0];
            ice::pod::array::push_back(events_out, event);
        }
        if (_position_relative[1] != 0)
        {
            event.identifier = input_identifier(DeviceType::Mouse, MouseInput::PositionYRelative);
            event.value.axis.value_i32 = _position_relative[1];
            ice::pod::array::push_back(events_out, event);
        }

        if (_wheel != 0)
        {
            event.identifier = input_identifier(DeviceType::Mouse, MouseInput::Wheel);
            event.value.axis.value_i32 = _wheel;
            ice::pod::array::push_back(events_out, event);
        }

        for (detail::ControlState& control : _controls)
        {
            if (detail::prepared_input_event(control, event))
            {
                ice::pod::array::push_back(events_out, event);
            }
        }
    }

    auto create_mouse_device(
        ice::Allocator& alloc,
        ice::input::DeviceHandle device
    ) noexcept -> ice::input::InputDevice*
    {
        return alloc.make<MouseDevice>(alloc, device);
    }

} // namespace ice::input
