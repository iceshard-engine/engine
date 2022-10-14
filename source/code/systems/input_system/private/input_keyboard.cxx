#include "input_devices.hxx"
#include "input_state_helpers.hxx"

#include <ice/input/input_keyboard.hxx>
#include <ice/container/array.hxx>

namespace ice::input
{

    static constexpr ice::u32 keyboard_key_num = static_cast<ice::u32>(KeyboardKey::Reserved);
    static constexpr ice::u32 keyboard_mod_num = 11;

    class KeyboardDevice : public InputDevice
    {
    public:
        KeyboardDevice(
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
            ice::Array<ice::input::InputEvent>& events_out
        ) noexcept override;

    private:
        ice::input::DeviceHandle _device;
        ice::Array<detail::ControlState> _controls;
    };

    namespace detail
    {

        constexpr auto input_control_index(ice::input::InputID input) noexcept -> ice::u32
        {
            switch (input)
            {
            case input_identifier(DeviceType::Keyboard, KeyboardMod::ShiftLeft, mod_identifier_base_value):
                return keyboard_key_num + 0;
            case input_identifier(DeviceType::Keyboard, KeyboardMod::ShiftRight, mod_identifier_base_value):
                return keyboard_key_num + 1;
            case input_identifier(DeviceType::Keyboard, KeyboardMod::CtrlLeft, mod_identifier_base_value):
                return keyboard_key_num + 2;
            case input_identifier(DeviceType::Keyboard, KeyboardMod::CtrlRight, mod_identifier_base_value):
                return keyboard_key_num + 3;
            case input_identifier(DeviceType::Keyboard, KeyboardMod::AltLeft, mod_identifier_base_value):
                return keyboard_key_num + 4;
            case input_identifier(DeviceType::Keyboard, KeyboardMod::AltRight, mod_identifier_base_value):
                return keyboard_key_num + 5;
            case input_identifier(DeviceType::Keyboard, KeyboardMod::GuiLeft, mod_identifier_base_value):
                return keyboard_key_num + 6;
            case input_identifier(DeviceType::Keyboard, KeyboardMod::GuiRight, mod_identifier_base_value):
                return keyboard_key_num + 7;
            case input_identifier(DeviceType::Keyboard, KeyboardMod::CapsLock, mod_identifier_base_value):
                return keyboard_key_num + 8;
            case input_identifier(DeviceType::Keyboard, KeyboardMod::NumLock, mod_identifier_base_value):
                return keyboard_key_num + 9;
            case input_identifier(DeviceType::Keyboard, KeyboardMod::Mode, mod_identifier_base_value):
                return keyboard_key_num + 10;
            default:
                return input_identifier_value(input);
            }
        }

    } // namespace detail

    KeyboardDevice::KeyboardDevice(
        ice::Allocator& alloc,
        ice::input::DeviceHandle device
    ) noexcept
        : InputDevice{ }
        , _device{ device }
        , _controls{ alloc }
    {
        ice::array::resize(_controls, keyboard_key_num + keyboard_mod_num + 10);
        for (detail::ControlState& control : _controls)
        {
            control.id = InputID::Invalid;
            control.tick = 0;
            control.value.button.value_i32 = 0;
        }
    }

    void KeyboardDevice::on_tick(ice::Timer const& timer) noexcept
    {
        for (detail::ControlState& control : _controls)
        {
            detail::handle_value_button_hold_and_repeat(control);
        }
    }

    void KeyboardDevice::on_event(
        ice::input::DeviceEvent event,
        ice::Data payload
    ) noexcept
    {
        InputID input = InputID::Invalid;

        switch (event.message)
        {
        case DeviceMessage::KeyboardButtonDown:
        case DeviceMessage::KeyboardButtonUp:
            input = input_identifier(
                DeviceType::Keyboard,
                detail::read_one<KeyboardKey>(event, payload),
                key_identifier_base_value
            );
            break;
        case DeviceMessage::KeyboardModifierDown:
        case DeviceMessage::KeyboardModifierUp:
            input = input_identifier(
                DeviceType::Keyboard,
                detail::read_one<KeyboardMod>(event, payload),
                mod_identifier_base_value
            );
            break;
        default:
            break;
        }

        ice::u32 const control_index = detail::input_control_index(input);
        // #todo assert control_index < ice::pod::array::size(_controls)

        detail::ControlState control = _controls[control_index];
        control.id = input;

        if (event.message == DeviceMessage::KeyboardButtonDown)
        {
            detail::handle_value_button_down(control);
        }
        else if (event.message == DeviceMessage::KeyboardButtonUp)
        {
            detail::handle_value_button_up(control);
        }
        else if (event.message == DeviceMessage::KeyboardModifierDown)
        {
            detail::handle_value_button_down_simplified(control);
        }
        else if (event.message == DeviceMessage::KeyboardModifierUp)
        {
            detail::handle_value_button_up_simplified(control);
        }

        _controls[control_index] = control;
    }

    void KeyboardDevice::on_publish(
        ice::Array<ice::input::InputEvent>& events_out
    ) noexcept
    {
        ice::input::InputEvent event{
            .device = _device,
        };

        for (detail::ControlState& control : _controls)
        {
            if (detail::prepared_input_event(control, event))
            {
                ice::array::push_back(events_out, event);
            }
        }
    }

    auto create_keyboard_device(
        ice::Allocator& alloc,
        ice::input::DeviceHandle device
    ) noexcept -> ice::input::InputDevice*
    {
        return alloc.create<KeyboardDevice>(alloc, device);
    }

} // namespace ice::input
