#pragma once
#include <ice/input_action_definitions.hxx>

namespace ice
{

    struct InputActionLayerInfo
    {
        ice::u16 count_sources;
        ice::u16 count_actions;
        ice::u16 count_conditions;
        ice::u16 count_steps;
        ice::u16 count_modifiers;
        ice::u32 offset_strings;
    };

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

    struct InputActionInfo
    {
        //! \brief Offset and size of the name in the data blob.
        ice::ref16 name;

        //! \brief Data representation of the resulting Action. This defines with what type the resulting Shard will be sent.
        ice::InputActionDataType type;

        ice::InputActionBehavior behavior;

        ice::arr<2, ice::u16> conditions; // offset, count, steps_offset

        ice::arr<2, ice::u8> mods; // offset, count
    };

    static_assert(sizeof(InputActionInfo) % 4 == 0);

} // namespace ice
