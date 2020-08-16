#pragma once
#include <iceshard/input/input_event.hxx>
#include <core/pod/collections.hxx>

namespace iceshard::input
{

    static constexpr int32_t button_hold_tick_threshold = 30;
    static constexpr int32_t button_repeat_tick_threshold = 20;

    struct InputValueState
    {
        int32_t tick;
        InputValue value;
    };

    void handle_value_button_down(InputValueState& value) noexcept;
    void handle_value_button_hold(InputValueState& value) noexcept;
    void handle_value_button_up(InputValueState& value) noexcept;

    void handle_value_button_down_simplified(InputValueState& value) noexcept;
    void handle_value_button_up_simplified(InputValueState& value) noexcept;

    bool prepared_input_event(InputID input, InputValueState const& value, InputEvent& event) noexcept;

} // namespace iceshard::input
