#pragma once
#include <core/base.hxx>

#include <input/keyboard.h>
#include <input/utils/message_info.h>

namespace input::message
{

struct KeyDown
{
    input::Key key;
};

struct KeyUp
{
    input::Key key;
};

struct TextInput
{
    char text[32];
};

// Declare the above types as messages
DECLARE_MESSAGE(KeyDown);
DECLARE_MESSAGE(KeyUp);
DECLARE_MESSAGE(TextInput);

}
