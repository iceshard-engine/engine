#pragma once
#include <ice/mem_data.hxx>
#include <ice/input/input_event.hxx>
#include <ice/input/device_event.hxx>
#include <ice/span.hxx>

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


    template<typename T>
    auto read_internal(
        ice::input::DeviceEvent event,
        ice::Data data
    ) noexcept -> ice::Span<T const>
    {
        static constexpr ice::input::DeviceEventPayload payload = ice::input::detail::payload_info<T>;
        static_assert(payload.type != 0x0, "Input data type is not declared");
        static_assert(payload.size != 0x0, "Input data type has invalid size == 0");
        static_assert(payload.count == 0x0, "Input data type has invalid default count != 0");

        //IS_ASSERT(message.input_data.type == data_info.type, "Message data type does not match requested data type!");
        // #todo data is of invalid type

        T const* data_ptr = reinterpret_cast<T const*>(data.location);
        return { data_ptr, event.payload.count };
    }

    template<typename T>
    auto read(
        ice::input::DeviceEvent event,
        ice::Data payload
    ) noexcept
    {
        return read_internal<T>(event, payload);
    }

    template<typename T>
    auto read_one(
        ice::input::DeviceEvent event,
        ice::Data payload
    ) noexcept
    {
        if constexpr (std::is_enum_v<T>)
        {
            return T{ read<std::underlying_type_t<T>>(event, payload)[0] };
        }
        else
        {
            return read<T>(event, payload)[0];
        }
    }

} // namespace ice::input::detail
