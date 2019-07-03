#pragma once
#include <core/base.hxx>

#include <input/keyboard.h>
#include <input/utils/message_info.h>

namespace mooned::io::message
{

struct KeyDown
{
    mooned::io::Key key;
};

struct KeyUp
{
    mooned::io::Key key;
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
