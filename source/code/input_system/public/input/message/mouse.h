#pragma once
#include <core/base.hxx>

#include <input/mouse.h>
#include <input/utils/message_info.h>

namespace input::message
{

struct MouseMotion
{
    int x, y;
};

struct MouseMotionRelative
{
    int dx, dy;
};

struct MouseButtonDown
{
    MouseButton button;
};

struct MouseButtonUp
{
    MouseButton button;
};

struct MouseWheel
{
    int y;
};

// Declare the above types as messages
DECLARE_MESSAGE(MouseMotion);
DECLARE_MESSAGE(MouseMotionRelative);
DECLARE_MESSAGE(MouseButtonDown);
DECLARE_MESSAGE(MouseButtonUp);
DECLARE_MESSAGE(MouseWheel);

}
