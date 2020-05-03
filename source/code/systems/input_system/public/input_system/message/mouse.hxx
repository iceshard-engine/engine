#pragma once
#include <core/cexpr/stringid.hxx>
#include <input_system/mouse.hxx>

namespace input::message
{

    struct MouseMotion
    {
        static constexpr auto message_type = "Mouse.Motion"_sid;

        bool relative;
        MousePos pos;
    };

    struct MouseButtonDown
    {
        static constexpr auto message_type = "Mouse.Button.Down"_sid;

        MouseButton button;
        MousePos pos;
    };

    struct MouseButtonUp
    {
        static constexpr auto message_type = "Mouse.Button.Up"_sid;

        MouseButton button;
        MousePos pos;
    };

    struct MouseWheel
    {
        static constexpr auto message_type = "Mouse.Wheel"_sid;

        int32_t dx;
        int32_t dy;
    };

} // namespace input::message
