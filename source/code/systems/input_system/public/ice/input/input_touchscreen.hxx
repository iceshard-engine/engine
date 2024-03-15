/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>
#include <ice/math.hxx>

namespace ice::input
{

    static constexpr ice::i32 Constant_TouchScreenClickThreshold = 35;
    static constexpr ice::i32 Constant_TouchScreenHoldThreshold = 36;

    enum class TouchScreenGestureType : ice::u8
    {
        Simple, // Lines, curves (7x7)
        Complex, // Splines, Spirals (7x7)
        Detailed, // Splines, Spirals (15x15)
    };

    struct TouchScreenGesture
    {
        ice::f32 matrix_weights[7*7] {
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,

            0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,

            0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,

            0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,

            0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,

            0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,

            0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        };
    };

    enum class TouchScreenVirtualControllerType : ice::u8
    {
        Button,
        Trigger,
        Axis,
    };

    struct TouchScreenVirtualController
    {
        ice::input::TouchScreenVirtualControllerType type;
        ice::vec4f area; // 0.f -> 1.f (center + size)
        // TODO: area value to final value function transform.
        union
        {
            ice::u32 vval_u32;
            ice::f32 vval_f32;
        };
    };

    enum class TouchInput : ice::u16
    {
        Unknown,

        // Virtual controls
        VirtualButton,

        // Touch pointer values
        TouchPointerCount,
        TouchStart,
        TouchPosX,
        TouchPosY,
        TouchEnd,

        GestureSimple,
        GestureComplex,
        GestureDetailed,

        VirtualLeftAxisX = 0x01'00,
        VirtualLeftAxisY,
        VirtualRightAxisX,
        VirtualRightAxisY,

        VirtualLeftTrigger,
        VirtualRightTrigger,
    };

} // ice::input
