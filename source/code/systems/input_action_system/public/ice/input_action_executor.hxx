#pragma once
#include <ice/input_action_types.hxx>

namespace ice
{

    //! \brief Helper object to execute conditions, steps and modifiers on action objects and values.
    //! \details This class was introduced to properly handle some specific conditions / input sources (ex.: Axis) where
    //!   we need to check the value against a provided deadzone. Because this value might be set on a subsequent layer, the
    //!   `InputActionStack` object will prepare the executor before using on each action in each currently activated layer.
    //!
    //! \note Additional context specific features might be added. One of the current ideas would be custom constants,
    //!   that could be set by the a user and applied on a different layer.
    class InputActionExecutor
    {
    public:
        ~InputActionExecutor() noexcept = default;

        //! \brief Checks given condition against the input source and provided user value.
        //! \details This method is only called for conditions that are called on input sources.
        //! \param[in] condition ID of the condition to be executed.
        //! \param[in] input_source The input source that is referenced by this condition.
        //! \param[in] param User provided parameter that may be used to complete the check. (Not all conditions will make use this value)
        //! \return `true` if the condition was valid and passed the check, `false` otherwise.
        bool execute_condition(
            ice::InputActionCondition condition,
            ice::InputActionSource const& input_source,
            ice::f32 param
        ) const noexcept;

        //! \brief Checks given condition against an action and provided user value.
        //! \details This method is only called for conditions that are called on itself or other input actions.
        //! \param[in] condition ID of the condition to be executed.
        //! \param[in] action Runtime data of the referenced input action.
        //! \param[in] param User provided parameter that may be used to complete the check. (Not all conditions will make use this value)
        //! \return `true` if the condition was valid and passed the check, `false` otherwise.
        //!
        //! \note Input actions are executed in order of definition, so referencing an action that is defined later,
        //!   might result in undefined behavior.
        bool execute_condition(
            ice::InputActionCondition condition,
            ice::InputActionRuntime const& action,
            ice::f32 param
        ) const noexcept;

        //! \brief Executes a step over the current actions runtime object, this is generally called for '.activate', '.toggle' and
        //!   simmilar actions.
        //! \param[in] step Action step to be executed.
        //! \param[out] runtime Object to be modifed from the action step taken.
        void execute_step(
            ice::InputActionStep step,
            ice::InputActionRuntime& runtime
        ) const noexcept;

        //! \brief Executes a step by taking a value from an input source and storing it at a specific input action value.
        //! \param[in] step Action step to be executed.
        //! \param[in] source_value Input source from which we want to take the new value.
        //! \param[out] destination_value Destination value where we want to store the value from the input source.
        void execute_step(
            ice::InputActionStep step,
            ice::InputActionSource const& source_value,
            ice::f32& destination_value
        ) const noexcept;

        //! \brief Executes an modifier with a user provided parameter over an action value.
        //! \param modifier Modifier to be executed on the given value.
        //! \param action_value The value to be modified (if necessary).
        //! \param param User parameter that may be used by the selected modifier.
        void execute_modifier(
            ice::InputActionModifier modifier,
            ice::f32& action_value,
            ice::f32 param
        ) const noexcept;
    };

} // namespace ice
