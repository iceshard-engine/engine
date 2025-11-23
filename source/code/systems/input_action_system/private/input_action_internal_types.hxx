/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/input_action_definitions.hxx>
#include <ice/input_action_info.hxx>

namespace ice
{

    //! \brief Header-like structure for binarized version of an InputActionLayer.
    //! \note This structure will be used for storing input actions in binary format for release builds.
    struct InputActionLayerInfoHeader
    {
        ice::u8 size_name;
        ice::u8 count_constants;
        ice::u16 count_sources;
        ice::u16 count_actions;
        ice::u16 count_conditions;
        ice::u16 count_steps;
        ice::u16 count_modifiers;
        ice::u32 offset_strings;
    };

    struct InputActionIndex
    {
        static constexpr ice::u16 SelfIndex = 8191;

        ice::u16 source_index : 13;
        ice::u16 source_axis : 3;
    };

    static_assert(sizeof(ice::InputActionIndex) == sizeof(ice::u16));

    struct InputActionConditionData
    {
        ice::InputActionIndex source;
        ice::InputActionCondition id;
        ice::InputActionConditionFlags flags;
        ice::ref16 steps;
        ice::f32 param;
    };

    static_assert(sizeof(InputActionConditionData) % 4 == 0);

    struct InputActionStepData
    {
        ice::InputActionIndex source;
        ice::InputActionStep id;
        ice::u8 dst_axis;
    };

    static_assert(sizeof(InputActionStepData) % 4 == 0);

    struct InputActionModifierData
    {
        // \brief The modifier to be applied to the final values stored in the action.
        ice::InputActionModifier id;

        ice::u8 axis; // 0b0000'0[111] (0000'0[zyx])

        //! \brief Offset where additional parameter are stored for float conditions.
        ice::f32 param;
    };

    static_assert(sizeof(InputActionModifierData) % 4 == 0);

} // namespace ice
