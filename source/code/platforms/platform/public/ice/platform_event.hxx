#pragma once
#include <ice/span.hxx>
#include <ice/shard.hxx>

namespace ice::platform
{

    struct Event;

    enum class EventType : ice::u32;

    enum class EventType : ice::u32
    {
        AppQuit = 1,
        WindowResized,
        WindowSizeChanged = WindowResized,

        // [issue #33]
        InputText,

        // Not implemented
        AppTerminate,
        AppSuspending,
        AppSuspended,
        AppResuming,
        AppResumed,
        WindowClosed,
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
            struct
            {
                ice::String text;
            } input;
        } data;
    };

    static constexpr ice::Shard Shard_WindowSizeChanged = "event/window/size-changed"_shard;

} // namespace ice::platform
