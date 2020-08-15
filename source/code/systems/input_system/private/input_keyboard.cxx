#include <iceshard/input/input_keyboard.hxx>
#include <iceshard/input/input_state_manager.hxx>
#include <core/pod/hash.hxx>

#include "input_state_helpers.hxx"

namespace iceshard::input
{

    static constexpr auto KeyboardKeyNum = static_cast<uint32_t>(KeyboardKey::NumPad0) + 1;

    class KeyboardDeviceState : public DeviceState
    {
    public:
        KeyboardDeviceState(core::allocator& alloc, DeviceHandle device) noexcept;

        void on_tick() noexcept override;
        void on_message(DeviceInputMessage const msg, void const* data) noexcept override;
        void on_publish(core::pod::Array<InputEvent>& events_out) noexcept override;

    private:
        DeviceHandle _device;
        core::pod::Hash<InputValueState> _inputs;
    };

    KeyboardDeviceState::KeyboardDeviceState(core::allocator& alloc, DeviceHandle device) noexcept
        : _device{ device }
        , _inputs{ alloc }
    {
    }

    void KeyboardDeviceState::on_tick() noexcept
    {
        for (auto& input : _inputs)
        {
            handle_value_button_hold(input.value);
        }
    }

    void KeyboardDeviceState::on_message(DeviceInputMessage const msg, void const* data) noexcept
    {
        InputID input = InputID::Invalid;

        switch (msg.input_type)
        {
        case DeviceInputType::KeyboardButtonDown:
        case DeviceInputType::KeyboardButtonUp:
            input = create_inputid(DeviceType::Keyboard, read_one<KeyboardKey>(msg, data));
            break;
        case DeviceInputType::KeyboardModifierDown:
        case DeviceInputType::KeyboardModifierUp:
            input = create_inputid(DeviceType::Keyboard, read_one<KeyboardMod>(msg, data));
            break;
        default:
            break;
        }

        auto const input_hash = core::hash(input);
        auto input_value = core::pod::hash::get(
            _inputs,
            input_hash,
            InputValueState{ }
        );

        if (msg.input_type == DeviceInputType::KeyboardButtonDown)
        {
            handle_value_button_down(input_value);
        }
        else if (msg.input_type == DeviceInputType::KeyboardButtonUp)
        {
            handle_value_button_up(input_value);
        }
        else if (msg.input_type == DeviceInputType::KeyboardModifierDown)
        {
            handle_value_button_down_simplified(input_value);
        }
        else if (msg.input_type == DeviceInputType::KeyboardModifierUp)
        {
            handle_value_button_up_simplified(input_value);
        }

        core::pod::hash::set(_inputs, input_hash, input_value);
    }

    void KeyboardDeviceState::on_publish(core::pod::Array<InputEvent>& events_out) noexcept
    {
        InputEvent event;
        event.device = static_cast<InputDevice>(_device);

        for (auto const& input : _inputs)
        {
            prepared_input_event(InputID{ input.key }, input.value, event);
            core::pod::array::push_back(events_out, event);
        }
    }

    auto default_keyboard_state_factory(core::allocator& alloc, DeviceHandle device) noexcept -> DeviceState*
    {
        return alloc.make<KeyboardDeviceState>(alloc, device);
    }

} // namespace iceshard::input
