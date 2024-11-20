#pragma once
#include <ice/input_action_definitions.hxx>

namespace ice
{

    struct InputActionIndex
    {
        ice::u16 source_index : 13;
        ice::u16 source_axis : 3;
    };

    static_assert(sizeof(ice::InputActionIndex) == sizeof(ice::u16));

    struct InputActionConditionData
    {
        ice::InputActionIndex source;
        ice::InputActionCondition id;
        ice::InputActionConditionFlags flags;
        ice::arr<2, ice::u16> steps;
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
