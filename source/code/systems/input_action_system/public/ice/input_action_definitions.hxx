/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/clock_types.hxx>
#include <ice/input_action_types.hxx>

namespace ice
{

    //! \brief Developer defined input action that can represent anything from a simple click,
    //!   down to specific axis values and mixes between them.
    struct InputAction
    {
        //! \brief Provides a quick way to disable or enable specific actions during runtime. (default: true)
        bool enabled = true;

        //! \brief Tracks activity of the action. (default: false)
        //! \note An action can only be active if it's enabled.
        bool active = false;

        //! \brief The value the action is returning. This value is defined / calculated by a set of steps
        //!   the developer created for this specific action.
        //! \note The value is up-to-date / valid only when the action is active.
        ice::vec2f value;

        //! \brief Timestamp when the action was activated.
        //! \note The value is up-to-date / valid only when then action is active.
        ice::Timestamp timestamp;
    };

    //! \brief Events input sources can be updated by.
    enum class InputActionSourceEvent : ice::u8
    {
        //! \brief Input source was not updated in this frame.
        None,

        //! \brief Input source value was updated by a key press event.
        KeyPress,

        //! \brief Input source value was updated by a key release event.
        KeyRelease,

        //! \brief \copybrief KeyPress
        //! \note Alias more suitable for controllers.
        ButtonPress = KeyPress,

        //! \brief \copybrief KeyRelease
        //! \note Alias more suitable for controllers.
        ButtonRelease = KeyRelease,

        //! \brief Input source was updated by a controller trigger. (can represent any one-dimensional floating point input)
        Trigger,

        //! \brief Input source was updated by a controller stick input. (can represent any two-dimensional floating point input)
        //! \note This event will only be captured if the value is \b outside the deadzone.
        Axis,

        //! \brief Input source was updated by a controller stick input. (can represent any two-dimensional floating point input)
        //! \note This event will only be captured if the value is \b inside the deadzone.
        AxisDeadzone
    };

    //! \brief Developer defined "input source" for activating actions and processing their values.
    //! \details A source represents a single device input event. This can be a Button, Axis, Trigger, Key, Position on
    //!   any input device like a mouse, keyboard, controller or anything else.
    //!
    //! \note When represeting an axis, only a single component is set each source.
    //!   So for a 2d axis you will need to define two sources.
    //! \todo Try to remove the 23bit padding.
    struct InputActionSource
    {
        //! \brief The value of the input source.
        //! \details See \see InputActionSourceInfo::type for details.
        ice::f32 value;

        //! \brief The most recent event that triggered a value change for this source.
        //! \note It might be possible for multiple events of different types, to trigger an event.
        ice::InputActionSourceEvent temp_event;

        //! \brief The final type of the event that triggered a value change for this source.
        //! \note It might be possible for multiple events of different types, to trigger an event.
        ice::InputActionSourceEvent event;

        // 2 bytes of padding
    };

    static_assert(sizeof(InputActionSource) == 8);

    //! \brief Runtime representation of an action, required to handle internal state and value changes.
    struct InputActionRuntime : ice::InputAction
    {
        //! \copydoc InputActionInfo::type
        ice::InputActionDataType type;

        //! \brief Internal state value used to track action activation changes.
        ice::u8 state = 0;

        //! \brief Tracks if the action is currently enabled. (only for toggled actions)
        bool toggle_enabled = false;

        //! \brief Tracks if an action was enabled in the previous frame.
        bool was_active = false;

        //! \brief The value stored before modifiers are applied.
        ice::vec3f raw_value;

        //! \brief Final name of the action, combined with the 'stack' prefix.
        ice::String name;
    };

    //! \brief Conditions that can be executed by an action.
    enum class InputActionCondition : ice::u8
    {
        Invalid = 0,

        //! \brief Source is active. The source was updated by an input event this frame.
        Active,

        //! \brief Source was updated by a key / button press.
        Pressed,

        //! \brief Source was updated by a key / button release.
        Released,

        //! \brief Source was updated by a change in the reported trigger value.
        Trigger,

        //! \brief Source was updated by a change in the reported stick position. (outside of the deadzone)
        Axis,

        //! \brief Source was updated by a change in the reported stick position. (inside of the deadzone)
        AxisDeadzone,


        //! \brief Source value is greater than provided user value. _(source > param)_
        Greater,

        //! \brief Source value is greater or equal to provided user value. _(source >= param)_
        GreaterOrEqual,

        //! \brief Source value is lower than provided user value. _(source < param)_
        Lower,

        //! \brief Source value is lower or equal to provided user value. _(source <= param)_
        LowerOrEqual,

        //! \brief Source value is equal to provided user value. _(source == param)_
        Equal,

        //! \brief Source value is different than provided user value. _(source != param)_
        NotEqual,


        //! \brief Checks action if it's enabled and can be activated.
        //! \note Does not check if action is currently active.
        ActionEnabled,

        //! \brief Checks if action is active via a toggle.
        //! \note Check is only valid on toggled actions.
        ActionToggleActive,

        //! \brief Checks if action is inactive via a toggle.
        //! \note Check is only valid on toggled actions.
        ActionToggleInactive,

        //! \brief Always true, can be used to force some steps at the end of an action.
        //! \note One use case would be to reset the action's values.
        AlwaysTrue,
    };

    enum class InputActionConditionFlags : ice::u8
    {
        None = 0,

        //! @brief Performs operation 'series_success |= condition_success'
        SeriesOr = None,

        //! @brief Performs operation 'series_success &= condition_success'
        SeriesAnd = 0x01,

        //! @brief Checks the series status instead of condition status for 'RunSteps', 'Activate' and 'Final' operations.
        SeriesCheck = 0x02,

        //! @brief Finishes series of conditions.
        SeriesFinish = 0x04,

        //! @brief Runs all steps attached to a condition.
        RunSteps = 0x08,

        // //! @brief Activates an action if the condition/series are successful. Implies 'RunSteps'.
        // Activate = 0x10,

        // //! @brief Deactivates an action if the condition/series are successful. Implies 'RunSteps'.
        // Deactivate = 0x20,

        //! @brief Stops execution conditions for this action if the series are successful. Implies 'SeriesFinish'.
        Final = 0x80 | SeriesFinish,
    };

    //! \brief Steps that can be executed after a condition (or series of conditions) was evaluated successfuly.
    enum class InputActionStep : ice::u8
    {
        Invalid = 0,

        //! \brief Activates an action for the current frame.
        //! \note If used in a 'toggled' action, activation will still only activate it for a single frame.
        //!   If the action is already 'active' via toggling, then nothing changes.
        Activate,

        //! \brief Deactivates an action for all following frames.
        //! \note Generally not necessary to be added, since actions are only active for a single frame.
        //!   If used on a 'toggled' it will deactivate the toggle.
        Deactivate,

        //! \brief Used to implement 'toggled' actions, allowing an action to be active even when no conditions are passed.
        Toggle,

        //! \brief Resets internal input action values to their defaults.
        Reset,

        //! \brief Sets the input actions value to "time passed since activation" (in seconds)
        Time,

        //! \brief Sets the input actions value to the value reported by a given source.
        Set,

        //! \brief Increases the input actions value by the value reported by a given source.
        Add,

        //! \brief Reduces the input actions value by the value reported by a given source.
        Sub,
    };

    //! \brief Modifiers that can be applied to action values before publishing.
    //! \details Each modifier is re-evaluated every frame as long as the action is active.
    //!   However because actions use a double-storage approach (raw -> final) the modifiers will not result in infinite
    //!   divisions (or other actions) of the previous value.
    enum class InputActionModifier : ice::u8
    {
        Invalid = 0,

        //! \brief Adds param-value from action-value. `action_value - user_param`
        Add,
        //! \brief Subtracts param-value from action-value. `action_value - user_param`
        Sub,
        //! \brief Multiplies action-value by param-value. `action_value * user_param`
        Mul,
        //! \brief Divides action-value by param-value. `action_value / user_param`
        //! \note The value cannot be '0'!
        Div,

        //! \brief Higher value of the actions value and a user provided param. `max(action_value, user_param)`
        MaxOf,

        //! \brief Lower value of the actions value and a user provided param. `min(action_value, user_param)`
        MinOf,
    };

    //! \brief Data representation of active input actions.
    //! \details When using 'ActionObject' (or 'object' in scripts) the user will receive all available information on that specific
    //!   action. However because values are stored internally as a 'ice::vec2f' interpretation needs to be done by the consumer.
    //!
    //! \note This value only affects how data is published to the rest of the engine (using a shard) once an action
    //!   is active. It does not change the internal storage of actual action data.
    enum class InputActionDataType : ice::u8
    {
        Invalid = 0,
        Bool,
        Float1,
        Float2,
        ActionObject,

        //! \todo Added for possible support of VR inputs in the future.
        // Float3,
    };

    //! \brief The check to be performed on an existing action.
    //! \note Only used as argument for 'InputActionStack::action_check'
    enum class InputActionCheck : ice::u8
    {
        None = 0,
        Exists = None,
        Enabled,
        Disabled,
        Active,
        Inactive,
    };

} // namespace ice
