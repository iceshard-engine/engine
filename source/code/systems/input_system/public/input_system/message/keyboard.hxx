#pragma once
#include <core/cexpr/stringid.hxx>

namespace input::message
{

    struct TextInput
    {
        static constexpr auto message_type = "Text.Input"_sid;

        char text[32];
    };

} // namespace input::message
