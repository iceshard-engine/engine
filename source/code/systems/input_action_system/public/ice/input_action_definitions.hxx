#pragma once
#include <ice/clock_types.hxx>
#include <ice/input_action_types.hxx>

namespace ice
{

    struct InputActionSource
    {
        ice::InputActionSourceEvent event;
        ice::f32 value;
        bool changed;
    };

    struct InputActionLayerInfo
    {
        ice::u16 count_sources;
        ice::u16 count_actions;
        ice::u16 count_conditions;
        ice::u16 count_steps;
        ice::u16 count_modifiers;
        ice::u32 offset_strings;
    };

    struct InputActionSourceInfo
    {
        //! \brief Where to find the name in the data blob.
        ice::u16 name_offset, name_length;

        //! \brief Input identifier this input source object will get values from.
        ice::input::InputID input;

        //! \brief Action Type, which determines how input events are processed.
        //!
        //! \note Behavior definitions:
        //!   `Key` | `Button` => `1.0` on Press, `0.0` on Release.
        //!   `Axis` => `if <value> gt_or_eq <deadzone>` then `[deadzone, 1.0]`, else `0.0`
        ice::InputActionSourceType type;

        //! \brief The storage offset where the input value should be stored.
        ice::u16 storage;

        //! \brief Additional param usable by some source types.
        //! \note Currently only used by 'Axis' sources for deadzone checks.
        ice::f32 param = 0.0;
    };

    static_assert(sizeof(InputActionSourceInfo) % 4 == 0);

    struct InputActionInfo
    {
        //! \brief Where to find the name in the data blob.
        ice::u16 name_offset, name_length;

        //! \brief Presentation of the action data.
        ice::InputActionData presentation;

        ice::InputActionBehavior behavior;

        ice::arr<2, ice::u16> conditions; // offset, count, steps_offset

        ice::arr<2, ice::u8> mods; // offset, count
    };

    static_assert(sizeof(InputActionInfo) % 4 == 0);

    struct InputActionRuntime
    {
        ice::Timestamp activation_ts;
        ice::vec3f action_value;
        ice::vec3f final_value;
        ice::u8 state = 0;
        bool toggle_enabled = false;
        bool enabled = false;
        bool was_active = false;
        bool active = false;
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

    enum class InputActionSourceType : ice::u8
    {
        Key,
        Button,
        Trigger,
        Axis1d = Trigger,
        Axis2d
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

        Pressed,
        Released,

        Trigger,
        Axis,

        AxisDeadzone,

        Greater,
        GreaterOrEqual,
        Lower,
        LowerOrEqual,
        Equal,

        Enabled,
        Active,
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

    enum class InputActionStep : ice::u8
    {
        Invalid = 0,

        // Action state change
        Enable,
        Disable,
        Toggle,

        // Action activation change
        Activate,
        Deactivate,

        // Data operations
        Set,
        Add,
        Sub,
    };

    enum class InputActionModifier : ice::u8
    {
        Invalid = 0,
        Div,
        Max,
    };

    enum class InputActionData : ice::u8
    {
        Invalid = 0,
        Bool,
        Float1,
        Float2,
        Float3,
    };


} // namespace ice
