#pragma once
#include <core/base.hxx>
#include <input/utils/message_info.h>

namespace mooned::io::message
{

struct WindowShown
{
    uint32_t handle;
};

struct WindowHidden
{
    uint32_t handle;
};

struct WindowExposed
{
    uint32_t handle;
};

struct WindowMinimized
{
    uint32_t handle;
};

struct WindowMaximized
{
    uint32_t handle;
};

struct WindowRestored
{
    uint32_t handle;
};

struct WindowMouseEntered
{
    uint32_t handle;
};

struct WindowMouseLeft
{
    uint32_t handle;
};

struct WindowFocusGained
{
    uint32_t handle;
};

struct WindowFocusLost
{
    uint32_t handle;
};

struct WindowClose
{
    uint32_t handle;
};

struct WindowTakeFocus
{
    uint32_t handle;
};

struct WindowHitTest
{
    uint32_t handle;
};

struct WindowMoved
{
    uint32_t handle;
    int x;
    int y;
};

struct WindowResized
{
    uint32_t handle;
    int width;
    int height;
};

struct WindowSizeChanged
{
    uint32_t handle;
    int width;
    int height;
};

// Declare the above types as messages
DECLARE_MESSAGE(WindowShown);
DECLARE_MESSAGE(WindowHidden);
DECLARE_MESSAGE(WindowExposed);

DECLARE_MESSAGE(WindowMinimized);
DECLARE_MESSAGE(WindowMaximized);
DECLARE_MESSAGE(WindowRestored);

DECLARE_MESSAGE(WindowMouseEntered);
DECLARE_MESSAGE(WindowMouseLeft);

DECLARE_MESSAGE(WindowFocusGained);
DECLARE_MESSAGE(WindowFocusLost);

DECLARE_MESSAGE(WindowClose);

DECLARE_MESSAGE(WindowTakeFocus);
DECLARE_MESSAGE(WindowHitTest);

DECLARE_MESSAGE(WindowMoved);
DECLARE_MESSAGE(WindowResized);
DECLARE_MESSAGE(WindowSizeChanged);

}
