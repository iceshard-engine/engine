#pragma once
#include <ice/ui_types.hxx>

namespace ice::ui
{

    enum class DataSource : ice::u16
    {
        None,
        Value,
        ValueResource,
        ValueProperty,
        ValueUIPage,
    };

    enum class ActionType : ice::u16
    {
        None,
        Data,
        Shard,
        UIShow,
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

        ice::ui::DataSource type_data;
        ice::u16 type_data_i;
    };

} // namespace ice::ui
