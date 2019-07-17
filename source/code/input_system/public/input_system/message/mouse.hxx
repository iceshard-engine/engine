#pragma once
#include <core/cexpr/stringid.hxx>


namespace input::message
{


    struct MouseMove
    {
        static inline const core::cexpr::stringid_type message_type{ core::cexpr::stringid("Mouse.Move") };

        int32_t x;
        int32_t y;
    };


} // namespace input::message
