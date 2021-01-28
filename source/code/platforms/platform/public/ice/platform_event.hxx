#pragma once
#include <ice/base.hxx>
#include <ice/span.hxx>

namespace ice::platform
{

    struct Event;

    enum class EventType : ice::u32;

    enum class EventType : ice::u32
    {
        AppQuit = 1,

        // Not implemented
        AppTerminate,
        AppSuspending,
        AppSuspended,
        AppResuming,
        AppResumed,
        WindowClosed,
        WindowResized,
        WindowSizeChanged = WindowResized,
        WindowMoved,
        WindowMinimized,
        WindowMaximized,
        WindowRestored,
        WindowFocusGained,
        WindowFocusLost,
        WindowFocusTake,
        WindowMouseEntered,
        WindowMouseLeft,
    };

    struct Event
    {
        EventType type;
        union
        {
            struct
            {
                ice::i32 reserved;
            } app;
            struct
            {
                union
                {
                    ice::vec2i pos;
                    ice::vec2i size;
                };
            } window;
        } data;
    };

} // namespace ice::platform
