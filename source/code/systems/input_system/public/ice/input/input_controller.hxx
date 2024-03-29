/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>

namespace ice::input
{

    enum class ControllerInput : ice::i16
    {
        Unknown = 0x0,

        ButtonA,
        ButtonB,
        ButtonX,
        ButtonY,
        ButtonUp,
        ButtonDown,
        ButtonLeft,
        ButtonRight,
        ButtonLeftShoulder,
        ButtonRightShoulder,

        LeftTrigger,
        RightTrigger,

        LeftAxisX,
        LeftAxisY,
        RightAxisX,
        RightAxisY,
    };

} // ice::input
