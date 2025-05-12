/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

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
