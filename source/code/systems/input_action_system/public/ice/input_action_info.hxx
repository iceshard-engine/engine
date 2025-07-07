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

    struct InputActionSourceEntryInfo
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

        //! \brief The storage offset where the input value should be stored.
        ice::u16 storage;

        //! \brief Additional param usable by some source types.
        //! \note Currently only used by 'Axis' sources for deadzone checks.
        ice::f32 param = 0.0;
    };

    static_assert(sizeof(InputActionSourceEntryInfo) % 4 == 0);

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

} // namespace ice
