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

    struct KeyboardModChanged
    {
        static constexpr auto message_type = "Mod.Changed"_sid;

        KeyboardMod mod;
        bool pressed;
    };

    struct KeyboardTextInput
    {
        static constexpr auto message_type = "Text.Input"_sid;

        char text[32];
    };

} // namespace input::message
