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

    //! \brief Developer defined "input source" for activating actions and processing their values.
    //! \details A source represents a single device input event. This can be a Button, Axis, Trigger, Key, Position on
    //!   any input device like a mouse, keyboard, controller or anything else.
    //! \note When represeting an axis, only a single component is set each source.
    //!   So for a 2d axis you will need to define two sources.
    struct InputActionSource
    {
        //! \brief The type of the event that triggered a value change for this source.
        //! \note It might be possible for multiple events of different types, to trigger an event.
        ice::InputActionSourceEvent event;

        //! \brief Tracks if the value actually changed between the action events.
        //! \note Since multiple events can trigger a value change, we want to act only once for multiple buttons.
        bool changed;

        //! \brief The value of the input source.
        //! \details See \see InputActionSourceInfo::type for details.
        ice::f32 value;
    };

    struct InputActionRuntime : ice::InputAction
    {
        ice::vec3f raw_value;
        ice::String name;

        ice::InputActionDataType type;
        ice::u8 state = 0;
        bool toggle_enabled = false;
        bool was_active = false;
    };

    static_assert(sizeof(InputActionRuntime) == 64);


    enum class InputActionCheck : ice::u8
    {
        None,
        Exists = None,
        Enabled,
        Disabled,
        Active,
        Inactive,
    };

    enum class InputActionSourceEvent : ice::u8
    {
        None,
        KeyPress,
        KeyRelease,
        ButonPress = KeyPress,
        ButonRelease = KeyRelease,
        Trigger,
        Axis,
        AxisDeadzone
    };

    enum class InputActionBehavior : ice::u8
    {
        Default,
        Accumulated,
        ActiveOnce,
        Toggled,
    };

    enum class InputActionCondition : ice::u8
    {
        Invalid = 0,

        // Source conditions
        Active,
        Pressed,
        Released,

        Trigger,
        Axis,
        AxisDeadzone,

        // Source param conditions
        Greater,
        GreaterOrEqual,
        Lower,
        LowerOrEqual,
        Equal,
        NotEqual,

        // Action and Special conditions
        ActionEnabled,
        ActionToggleActive,
        ActionToggleInactive,
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

        //! \brief Divides action value by a user provided param. `action_value / user_param`
        Div,

        //! \brief Highest value between the actions value and a user provided param. `max(action_value, user_param)`
        Max,
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


} // namespace ice
