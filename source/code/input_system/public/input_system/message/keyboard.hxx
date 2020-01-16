#pragma once
#include <core/cexpr/stringid.hxx>
#include <input_system/keyboard.hxx>

namespace input::message
{

    struct KeyboardKeyDown
    {
        static constexpr auto message_type = "Key.Down"_sid;

        KeyboardKey key;
    };

    struct KeyboardKeyUp
    {
        static constexpr auto message_type = "Key.Up"_sid;

        KeyboardKey key;
    };

} // namespace input::message
