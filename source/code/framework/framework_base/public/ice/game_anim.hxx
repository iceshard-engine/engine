/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/math.hxx>
#include <ice/stringid.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/input/input_types.hxx>

namespace ice
{

    static constexpr ice::StringID Constant_TraitName_SpriteAnimator
        = "ice.base-framework.trait-sprite-animator"_sid;

    struct Animation
    {
        static constexpr ice::StringID Identifier = "ice.component.animation"_sid;

        ice::StringID_Hash animation;
        ice::f32 speed = 1.f / 60.f;
    };

    struct AnimationState
    {
        static constexpr ice::StringID Identifier = "ice.component.animation-state"_sid;

        ice::StringID_Hash current_animation;
        ice::i64 timestamp;
        ice::vec2u frame;
    };

} // namespace ice
