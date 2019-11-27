#pragma once
#include <core/base.hxx>

namespace input
{

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
