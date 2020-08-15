#include "input_state_helpers.hxx"

namespace iceshard::input
{

    void handle_value_button_down(InputValueState& value) noexcept
    {
        auto& button = value.value.button;

        if (button.state_value == 0)
        {
            value.tick = 0;
            button.state.pressed = true;
        }
        else if (button.state.released)
        {
            value.tick = 0;
            button.state_value = 0;
            button.state.pressed = true;
        }
        else if (button.state.clicked || button.state.repeat > 0)
        {
            button.state.pressed = true;
        }
    }

    void handle_value_button_hold(InputValueState& value) noexcept
    {
        auto& button = value.value.button;

        if (button.state_value != 0 && button.state.hold == false)
        {
            value.tick += 1;
            if (value.tick > button_hold_tick_threshold)
            {
                if (button.state.pressed)
                {
                    value.tick = 0;
                    button.state.repeat = 0;
                    button.state.clicked = false;
                    button.state.hold = true;
                }
                else
                {
                    button.state_value = 0;
                }
            }
        }
    }

    void handle_value_button_up(InputValueState& value) noexcept
    {
        auto& button = value.value.button;

        if (button.state.hold)
        {
            value.tick = 0;
            button.state_value = 0;
            button.state.released = true;
        }
        else
        {
            if (button.state.repeat > 0)
            {
                value.tick = 0;
                button.state.repeat += 1;
                button.state.pressed = false;
            }
            else if (button.state.pressed)
            {
                if (value.tick <= button_repeat_tick_threshold)
                {
                    if (button.state.clicked)
                    {
                        button.state_value = 0;
                        button.state.repeat = 2;
                    }
                    else
                    {
                        button.state_value = 0;
                        button.state.clicked = true;
                    }
                }
                else
                {
                    button.state_value = 0;
                    button.state.released = true;
                }
                value.tick = 0;
            }
        }
    }

    void handle_value_button_down_simplified(InputValueState& value) noexcept
    {
        value.tick = 0;
        if (value.value.button.state_value == 0)
        {
            value.value.button.state.pressed = true;
        }
    }

    void handle_value_button_up_simplified(InputValueState& value) noexcept
    {
        if (value.value.button.state.pressed || value.value.button.state.hold)
        {
            value.tick = 0;
            value.value.button.state_value = 0;
            value.value.button.state.released = true;
        }
    }

    void prepared_input_event(InputID input, InputValueState const& value, InputEvent& event) noexcept
    {
        if (value.tick == 0)
        {
            event.identifier = input;
            event.value = value.value;
        }
        else if (value.value.button.state.pressed)
        {
            event.identifier = input;
            event.value.button.state_value = 0;
            event.value.button.state.pressed = true;
        }
    }

} // namespace iceshard::input
