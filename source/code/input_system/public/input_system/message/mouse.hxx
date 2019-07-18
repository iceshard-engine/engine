#pragma once
#include <core/cexpr/stringid.hxx>


namespace input::message
{


    struct MouseMotion
    {
        static inline const core::cexpr::stringid_type message_type{ core::cexpr::stringid("Mouse.Motion") };

        int32_t x;
        int32_t y;
    };


} // namespace input::message
