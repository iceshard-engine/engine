#include <iceshard/input/input_controller.hxx>
#include <iceshard/input/input_state_manager.hxx>
#include <core/pod/hash.hxx>

#include "input_state_helpers.hxx"

namespace iceshard::input
{

    class GamepadDeviceState : public DeviceState
    {
    public:
        GamepadDeviceState(core::allocator& alloc, DeviceHandle device) noexcept;

        void on_tick() noexcept override;
        void on_message(DeviceInputMessage msg, void const* data) noexcept override;
        void on_publish(core::pod::Array<InputEvent>& events_out) noexcept override;

    private:
        DeviceHandle _device;
        core::pod::Hash<InputValueState> _inputs;

        float _left_axis[2] = { 0, 0 };
        float _right_axis[2] = { 0, 0 };
        float _triggers[2] = { 0, 0 };

        bool _axis_reset_event[6] = { true, true, true, true, true, true };
    };

    GamepadDeviceState::GamepadDeviceState(core::allocator& alloc, DeviceHandle device) noexcept
        : _device{ device }
        , _inputs{ alloc }
    { }

    void GamepadDeviceState::on_tick() noexcept
    {
        auto reset_in_deadzone = [](float& val) noexcept
        {
            if (val <= 0.15f && val >= -0.15f)
            {
                val = 0.0f;
            }
        };

        reset_in_deadzone(_left_axis[0]);
        reset_in_deadzone(_left_axis[1]);
        reset_in_deadzone(_right_axis[0]);
        reset_in_deadzone(_right_axis[1]);
        reset_in_deadzone(_triggers[0]);
        reset_in_deadzone(_triggers[1]);

        for (auto& input : _inputs)
        {
            handle_value_button_hold(input.value);
        }
    }

    void GamepadDeviceState::on_message(DeviceInputMessage const msg, void const* data) noexcept
    {
        InputID input = InputID::Invalid;

        switch (msg.input_type)
        {
        case DeviceInputType::GamepadButtonDown:
        case DeviceInputType::GamepadButtonUp:
            input = create_inputid(DeviceType::Controller, read_one<ControllerInput>(msg, data));
            break;
        case DeviceInputType::GamepadTriggerLeft:
            _triggers[0] = read_one<float>(msg, data);
            return;
        case DeviceInputType::GamepadTriggerRight:
            _triggers[1] = read_one<float>(msg, data);
            return;
        case DeviceInputType::GamepadAxisLeft:
        {
            auto const axis_value = read<float>(msg, data);
            _left_axis[0] = axis_value[0];
            _left_axis[1] = axis_value[1];
            return;
        }
        case DeviceInputType::GamepadAxisLeftX:
            _left_axis[0] = read_one<float>(msg, data);
            return;
        case DeviceInputType::GamepadAxisLeftY:
            _left_axis[1] = read_one<float>(msg, data);
            return;
        case DeviceInputType::GamepadAxisRight:
        {
            auto const axis_value = read<float>(msg, data);
            _right_axis[0] = axis_value[0];
            _right_axis[1] = axis_value[1];
            return;
        }
        case DeviceInputType::GamepadAxisRightX:
            _right_axis[0] = read_one<float>(msg, data);
            return;
        case DeviceInputType::GamepadAxisRightY:
            _right_axis[1] = read_one<float>(msg, data);
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

        if (msg.input_type == DeviceInputType::GamepadButtonDown)
        {
            handle_value_button_down(input_value);
        }
        else if (msg.input_type == DeviceInputType::GamepadButtonUp)
        {
            handle_value_button_up(input_value);
        }

        core::pod::hash::set(_inputs, input_hash, input_value);
    }

    void GamepadDeviceState::on_publish(core::pod::Array<InputEvent>& events_out) noexcept
    {
        InputEvent event;
        event.device = static_cast<InputDevice>(_device);

        auto publish_axis_value = [&event, &events_out](ControllerInput input, float value, bool& reset) noexcept
        {
            if (value >= 0.25 || value <= -0.25f)
            {
                reset = false;
                event.identifier = create_inputid(DeviceType::Controller, input);
                event.value.axis.value_f32 = value;
                core::pod::array::push_back(events_out, event);
            }
            else if (reset == false)
            {
                reset = true;
                event.identifier = create_inputid(DeviceType::Controller, input);
                event.value.axis.value_f32 = 0.0f;
                core::pod::array::push_back(events_out, event);
            }
        };

        publish_axis_value(ControllerInput::LeftAxisX, _left_axis[0], _axis_reset_event[0]);
        publish_axis_value(ControllerInput::LeftAxisY, _left_axis[1], _axis_reset_event[1]);
        publish_axis_value(ControllerInput::RightAxisX, _right_axis[0], _axis_reset_event[2]);
        publish_axis_value(ControllerInput::RightAxisY, _right_axis[1], _axis_reset_event[3]);
        publish_axis_value(ControllerInput::LeftTrigger, _triggers[0], _axis_reset_event[4]);
        publish_axis_value(ControllerInput::RightTrigger, _triggers[1], _axis_reset_event[5]);

        for (auto& input : _inputs)
        {
            if (prepared_input_event(InputID{ input.key }, input.value, event))
            {
                core::pod::array::push_back(events_out, event);
            }
        }
    }

    auto default_controller_state_factory(core::allocator& alloc, DeviceHandle device) noexcept -> DeviceState*
    {
        return alloc.make<GamepadDeviceState>(alloc, device);
    }

} // namespace iceshard::input
