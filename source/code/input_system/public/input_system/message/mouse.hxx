#pragma once
#include <core/cexpr/stringid.hxx>
#include <input_system/mouse.hxx>

namespace input::message
{

    struct MousePos
    {
        int32_t x;
        int32_t y;
    };

    struct MouseMotion
    {
        static inline const core::cexpr::stringid_type message_type{ core::cexpr::stringid("Mouse.Motion") };

        MousePos pos;
    };

    struct MouseButtonDown
    {
        static inline const core::cexpr::stringid_type message_type{ core::cexpr::stringid("Mouse.Button.Down") };

        MouseButton button;
        MousePos pos;
    };

    struct MouseButtonUp
    {
        static inline const core::cexpr::stringid_type message_type{ core::cexpr::stringid("Mouse.Button.Up") };

        MouseButton button;
        MousePos pos;
    };

} // namespace input::message
