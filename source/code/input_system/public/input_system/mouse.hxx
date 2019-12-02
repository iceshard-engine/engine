#pragma once
#include <core/base.hxx>

namespace input
{

    struct MousePos
    {
        int32_t x;
        int32_t y;
    };

    //! \brief Supported mouse buttons.
    enum class MouseButton : uint32_t
    {
        Unknown

        , Left
        , Right
        , Middle

        , Custom0
    };

} // namespace input
