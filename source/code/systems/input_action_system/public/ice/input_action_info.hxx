#pragma once
#include <ice/input_action_definitions.hxx>

namespace ice
{

    //! \brief Source types which determine how events are processed to create action values.
    //!
    //! \note Behavior definitions:
    //!   `Key` | `Button` => `1.0` on Press, `0.0` on Release.
    //!   `Axis_d` => `if <value>.<component> gt_or_eq <deadzone>` then `[deadzone, 1.0]`, else `0.0`
    //! \note The `<deadzone>` value is also defined by a input action layer.
    enum class InputActionSourceType : ice::u8
    {
        Key,
        Button,
        Trigger,
        Axis1d,
        Axis2d
    };

    struct InputActionSourceInputInfo
    {
        //! \brief Offset and size of the name in the data blob.
        ice::ref16 name;

        //! \brief Native input ID input source object will get values from.
        ice::input::InputID input;

        //! \brief Action Type, which determines how input events are processed.
        //!
        //! \note Behavior definitions:
        //!   `Key` | `Button` => `1.0` on Press, `0.0` on Release.
        //!   `Axis` => `if <value> gt_or_eq <deadzone>` then `[deadzone, 1.0]`, else `0.0`
        ice::InputActionSourceType type;

        //! \brief Layer relative offset where the values from the event should be stored.
        ice::u16 storage_offset;

        //! \brief Additional param usable by some source types.
        //! \note Only used by 'Axis' sources for deadzone checks.
        //! \todo This param is not set or used, however it's here for future work.
        ice::f32 param;
    };

    static_assert(sizeof(InputActionSourceInputInfo) % 4 == 0);

    //! \brief Adds additional behavior constraints or features to an action.
    enum class InputActionBehavior : ice::u8
    {
        //! \brief Only active for as long the activation condition is true.
        Default,

        //! \brief Can be toggled to be active even when no 'activation' condition is true.
        //! \note A toggled action will still be behave as `Default` one, if the '.toggle' step is not used.
        Toggled,

        //! \brief Is active for a single frame after activation.
        //!   Requires the action to 'deactivate' before a new event will be sent.
        ActiveOnce,
    };

    struct InputActionInfo
    {
        //! \brief Offset and size of the name in the data blob.
        ice::ref16 name;

        //! \brief Data representation of the resulting Action. This defines with what type the resulting Shard will be sent.
        ice::InputActionDataType type;

        //! \brief Allows to customize behavior for actions.
        //! \details With the following:
        //!   * 'Default' - Action is active as long as it's activation condition is 'true'.
        //!   * 'Toggled' - Action is active after the activation condition was successful, and deactivated on the next success.
        //!   * 'Once' - Action is active only for a single frame after activation condition was successful.
        //!
        //! \note When defining actions in '.ias' scripts, it's better to use '.toggle' step instead of '.activate'
        //!   for additional readability, however the '.toggle' step is only allowed in 'toggled' input actions.
        ice::InputActionBehavior behavior;

        //! \brief Offset and count of conditions this action has defined.
        ice::ref16 conditions;

        //! \brief Offset and count of modifiers this action has defined.
        ice::ref8 mods;
    };

    static_assert(sizeof(InputActionInfo) % 4 == 0);

} // namespace ice
