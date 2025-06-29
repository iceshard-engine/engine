#pragma once
#include <ice/input_action_types.hxx>

namespace ice
{

    class InputActionExecutor
    {
    public:
        ~InputActionExecutor() noexcept = default;

        bool execute_condition(
            ice::InputActionCondition condition,
            ice::InputActionSource const& val,
            ice::f32 param
        ) const noexcept;

        bool execute_condition(
            ice::InputActionCondition condition,
            ice::InputActionRuntime const& action,
            ice::f32 param
        ) const noexcept;

        void execute_step(
            ice::InputActionStep step,
            ice::InputActionRuntime& runtime
        ) const noexcept;

        void execute_step(
            ice::InputActionStep step,
            ice::InputActionSource const& source_value,
            ice::f32& destination_value
        ) const noexcept;

        void execute_modifier(
            ice::InputActionModifier step,
            ice::f32& action_value,
            ice::f32 param
        ) const noexcept;
    };

} // namespace ice
