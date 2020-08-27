#include <iceshard/input/input_mouse.hxx>
#include <iceshard/input/input_state_manager.hxx>
#include <core/pod/hash.hxx>

#include "input_state_helpers.hxx"

namespace iceshard::input
{

    class MouseDeviceState : public DeviceState
    {
    public:
        MouseDeviceState(core::allocator& alloc, DeviceHandle device) noexcept;

        void on_tick() noexcept override;
        void on_message(DeviceInputMessage msg, void const* data) noexcept override;
        void on_publish(core::pod::Array<InputEvent>& events_out) noexcept override;

    private:
        DeviceHandle _device;
        core::pod::Hash<InputValueState> _inputs;

        int32_t _position[2] = { 0, 0 };
        int32_t _position_rel[2] = { 0, 0 };
        int32_t _wheel = 0;
    };

    MouseDeviceState::MouseDeviceState(core::allocator& alloc, DeviceHandle device) noexcept
        : _device{ device }
        , _inputs{ alloc }
    { }

    void MouseDeviceState::on_tick() noexcept
    {
        _wheel = 0;
        _position_rel[0] = 0;
        _position_rel[1] = 0;

        for (auto& input : _inputs)
        {
            handle_value_button_hold(input.value);
        }
    }

    void MouseDeviceState::on_message(DeviceInputMessage const msg, void const* data) noexcept
    {
        InputID input = InputID::Invalid;

        switch (msg.input_type)
        {
        case DeviceInputType::MouseButtonDown:
        case DeviceInputType::MouseButtonUp:
            input = create_inputid(DeviceType::Mouse, read_one<MouseInput>(msg, data));
            break;
        case DeviceInputType::MousePosition:
        {
            auto const pos = read<uint32_t>(msg, data);

            _position_rel[0] = pos[0] - _position[0];
            _position_rel[1] = pos[1] - _position[1];

            _position[0] = pos[0];
            _position[1] = pos[1];
            return;
        }
        case DeviceInputType::MouseWheel:
            _wheel = read_one<int32_t>(msg, data);
            return;
        default:
            return;
        }

        auto const input_hash = core::hash(input);
        auto input_value = core::pod::hash::get(
            _inputs,
            input_hash,
            InputValueState{ }
        );

        if (msg.input_type == DeviceInputType::MouseButtonDown)
        {
            handle_value_button_down(input_value);
        }
        else if (msg.input_type == DeviceInputType::MouseButtonUp)
        {
            handle_value_button_up(input_value);
        }

        core::pod::hash::set(_inputs, input_hash, input_value);
    }

    void MouseDeviceState::on_publish(core::pod::Array<InputEvent>& events_out) noexcept
    {
        InputEvent event;
        event.device = static_cast<InputDevice>(_device);

        event.identifier = create_inputid(DeviceType::Mouse, MouseInput::PositionX);
        event.value.axis.value_i32 = _position[0];
        core::pod::array::push_back(events_out, event);

        event.identifier = create_inputid(DeviceType::Mouse, MouseInput::PositionY);
        event.value.axis.value_i32 = _position[1];
        core::pod::array::push_back(events_out, event);

        if (_position_rel[0] != 0)
        {
            event.identifier = create_inputid(DeviceType::Mouse, MouseInput::PositionXRelative);
            event.value.axis.value_i32 = _position_rel[0];
            core::pod::array::push_back(events_out, event);
        }
        if (_position_rel[1] != 0)
        {
            event.identifier = create_inputid(DeviceType::Mouse, MouseInput::PositionYRelative);
            event.value.axis.value_i32 = _position_rel[1];
            core::pod::array::push_back(events_out, event);
        }

        if (_wheel != 0)
        {
            event.identifier = create_inputid(DeviceType::Mouse, MouseInput::Wheel);
            event.value.axis.value_i32 = _wheel;
            core::pod::array::push_back(events_out, event);
        }

        for (auto& input : _inputs)
        {
            if (prepared_input_event(InputID{ input.key }, input.value, event))
            {
                core::pod::array::push_back(events_out, event);
            }
        }
    }

    auto default_mouse_state_factory(core::allocator& alloc, DeviceHandle device) noexcept -> DeviceState*
    {
        return alloc.make<MouseDeviceState>(alloc, device);
    }

} // namespace iceshard::input
