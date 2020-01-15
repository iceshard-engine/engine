#pragma once
#include <core/cexpr/stringid.hxx>
#include <input_system/keyboard.hxx>

namespace input::message
{

    struct KeyboardKeyDown
    {
        static constexpr core::cexpr::stringid_type message_type = "Mouse.Motion"_sid;

    };

    struct KeyboardKeyUp
    {

    };

} // namespace input::message
