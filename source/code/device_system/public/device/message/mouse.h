#pragma once
#include <core/cexpr/stringid.hxx>


namespace driver::message
{


    struct MouseMove
    {
        static inline const core::cexpr::stringid_type message_type{ core::cexpr::stringid("Mouse.Move") };

        int32_t x;
        int32_t y;
    };


} // namespace driver::message

    /// OLD CODE ///

    //struct MouseMotion
    //{
    //    int x, y;
    //};

    //struct MouseMotionRelative
    //{
    //    int dx, dy;
    //};

    //struct MouseButtonDown
    //{
    //    MouseButton button;
    //};

    //struct MouseButtonUp
    //{
    //    MouseButton button;
    //};

    //struct MouseWheel
    //{
    //    int y;
    //};

    //// Declare the above types as messages
    //DECLARE_MESSAGE(MouseMotion);
    //DECLARE_MESSAGE(MouseMotionRelative);
    //DECLARE_MESSAGE(MouseButtonDown);
    //DECLARE_MESSAGE(MouseButtonUp);
    //DECLARE_MESSAGE(MouseWheel);

    /// OLD CODE ///


