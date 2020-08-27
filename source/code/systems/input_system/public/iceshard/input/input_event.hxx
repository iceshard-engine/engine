#pragma once
#include <core/utility.hxx>

namespace iceshard::input
{

    enum class DeviceType : uint8_t;

    enum class InputDevice : uint8_t
    {
        Invalid = 0x0,
    };

    enum class InputID : uint16_t
    {
        Invalid = 0x0,
    };

    inline constexpr auto create_inputid(DeviceType type, uint16_t value) noexcept -> InputID
    {
        return InputID{
            (static_cast<uint16_t>(type) << 12) | value
        };
    }

    union InputValue
    {
        union
        {
            float value_f32;
            int32_t value_i32;
        } axis;
        union
        {
            float value_range;
            struct
            {
                bool pressed : 1;
                bool hold : 1;
                bool released : 1;
                bool clicked : 1;
                uint16_t repeat;
            } state;
            int32_t state_value;
        } button;
    };

    static_assert(sizeof(InputValue) == 4);

    struct InputEvent
    {
        InputID identifier;
        InputDevice device;
        bool button_state : 1 = false;
        bool initial_event : 1 = true;
        InputValue value;
    };

    static_assert(sizeof(InputEvent) == 8);

} // namespace iceshard::input

template<>
constexpr inline auto core::hash<iceshard::input::InputID>(iceshard::input::InputID value) noexcept -> uint64_t
{
    return static_cast<uint64_t>(value);
}
