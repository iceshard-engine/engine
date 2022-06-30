#pragma once
#include <ice/ui_types.hxx>

namespace ice::ui
{

    enum class ActionType : ice::u16
    {
        None,
        Shard,
        UIShow,
    };

    enum class ActionData : ice::u16
    {
        None,
        ValueProperty,
        ValueUIPage,
    };

    enum class Property : ice::u16
    {
        None,
        Entity,
    };

    struct Action
    {
        ice::ui::ActionType type;
        ice::u16 type_i;

        ice::ui::ActionData type_data;
        ice::u16 type_data_i;
    };

} // namespace ice::ui
