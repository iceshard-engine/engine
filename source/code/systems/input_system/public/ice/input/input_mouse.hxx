#pragma once
#include <ice/base.hxx>

namespace ice::input
{

    enum class MouseInput : ice::i16
    {
        Unknown,

        ButtonLeft,
        ButtonRight,
        ButtonMiddle,

        ButtonCustom0,
        ButtonCustom1,

        PositionX,
        PositionY,
        PositionXRelative,
        PositionYRelative,
        Wheel,
    };

} // ice::input
