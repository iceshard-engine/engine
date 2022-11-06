/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_data.hxx>
#include <ice/input/input_event.hxx>
#include <ice/input/device_event.hxx>
#include <ice/span.hxx>
#include <ice/assert_core.hxx>

namespace ice::input::detail
{

    struct ControlState
    {
        ice::i32 tick;
        ice::input::InputID id;
        ice::input::InputValue value;
    };

    void handle_value_button_down(ControlState& value) noexcept;
    void handle_value_button_hold_and_repeat(ControlState& value) noexcept;
    void handle_value_button_up(ControlState& value) noexcept;

    void handle_value_button_down_simplified(ControlState& value) noexcept;
    void handle_value_button_up_simplified(ControlState& value) noexcept;

    bool prepared_input_event(
        ControlState& value,
        ice::input::InputEvent& event
    ) noexcept;


    template<typename T, DevicePayloadType PayloadType = ice::input::Constant_PayloadType<T>>
        requires (std::is_enum_v<T> == false)
    auto event_data(ice::input::DeviceEvent event) noexcept -> T
    {
        static_assert(PayloadType != ice::input::DevicePayloadType::Invalid);
        ICE_ASSERT_CORE(PayloadType == event.payload_type);

        if constexpr (std::is_same_v<T, ice::i8>)
        {
            return event.payload_data.val_i8;
        }
        else if constexpr (std::is_same_v<T, ice::i16>)
        {
            return event.payload_data.val_i16;
        }
        else if constexpr (std::is_same_v<T, ice::i32>)
        {
            return event.payload_data.val_i32;
        }
        else if constexpr (std::is_same_v<T, ice::f32>)
        {
            return event.payload_data.val_f32;
        }
    }

    template<typename T> requires std::is_enum_v<T>
    auto event_data(ice::input::DeviceEvent event) noexcept -> T
    {
        return static_cast<T>(event_data<std::underlying_type_t<T>, DevicePayloadType::Enum>(event));
    }

} // namespace ice::input::detail
