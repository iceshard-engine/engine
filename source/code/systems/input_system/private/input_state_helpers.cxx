/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "input_state_helpers.hxx"
#include <ice/input/input_tracker.hxx>

namespace ice::input::detail
{

    void handle_value_button_down(ControlState& value) noexcept
    {
        auto& button = value.value.button;

        value.tick = 0;
        if (button.state.clicked || button.state.repeat > 0)
        {
            value.tick = 1;
        }
        else if (button.state.released)
        {
            button.value_i32 = 0;
        }

        button.state.released = false;
        button.state.pressed = true;
    }

    void handle_value_button_hold_and_repeat(ControlState& value) noexcept
    {
        auto& button = value.value.button;

        if (button.value_i32 != 0)
        {
            value.tick += 1;
            if (value.tick > ice::input::default_button_event_treshold_hold)
            {
                if (button.state.pressed)
                {
                    value.tick = 0;
                    button.state.repeat = 0;
                    button.state.clicked = false;
                    button.state.released = false;
                    button.state.hold = true;
                }
                else
                {
                    button.value_i32 = 0;
                }
            }
            else if (value.tick > ice::input::default_button_event_treshold_repeat)
            {
                if (!button.state.pressed && (button.state.clicked || button.state.repeat > 0))
                {
                    button.value_i32 = 0;
                }
            }
        }
    }

    void handle_value_button_up(ControlState& value) noexcept
    {
        auto& button = value.value.button;

        if (button.state.hold)
        {
            button.value_i32 = 0;
        }
        else if (value.tick <= ice::input::default_button_event_treshold_repeat)
        {
            if (button.state.repeat > 0)
            {
                button.state.pressed = false;
                button.state.repeat += 1;
            }
            else if (button.state.clicked)
            {
                button.value_i32 = 0;
                button.state.repeat = 2;
            }
            else
            {
                button.value_i32 = 0;
                button.state.clicked = true;
            }
        }
        else if (value.tick <= ice::input::default_button_event_treshold_hold)
        {
            button.value_i32 = 0;
        }

        button.state.released = true;
        value.tick = 0;
    }

    void handle_value_button_down_simplified(ControlState& value) noexcept
    {
        value.tick = 0;
        if (value.value.button.value_i32 == 0)
        {
            value.value.button.state.pressed = true;
        }
    }

    void handle_value_button_up_simplified(ControlState& value) noexcept
    {
        if (value.value.button.state.pressed || value.value.button.state.hold)
        {
            value.tick = 0;
            value.value.button.value_i32 = 0;
            value.value.button.state.released = true;
        }
    }

    bool prepared_input_event(
        ControlState& value,
        ice::input::InputEvent& event
    ) noexcept
    {
        if (value.tick == 0)
        {
            event.identifier = value.id;
            event.initial_event = true;
            event.value_type = InputValueType::Button;
            event.value = value.value;
            value.tick = 1;
            return true;
        }
        else if (value.value.button.state.pressed)
        {
            event.identifier = value.id;
            event.initial_event = false;
            event.value_type = InputValueType::Button;
            event.value.button.value_i32 = 0;
            event.value.button.state.pressed = true;
            return true;
        }
        return false;
    }

} // namespace iceshard::input
