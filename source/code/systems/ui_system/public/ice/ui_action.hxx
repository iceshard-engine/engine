#pragma once
#include <ice/ui_types.hxx>
#include <ice/ui_data_ref.hxx>
#include <ice/string/string.hxx>
#include <ice/span.hxx>

namespace ice::ui
{

    enum class ActionType : ice::u16
    {
        None,
        Data,
        Shard,
        UIShow,
    };

    struct ActionInfo
    {
        ice::ui::ActionType type;
        ice::u16 type_i;

        ice::ui::DataRef data;
    };

    static_assert(sizeof(ActionInfo) == 8);

} // namespace ice::ui
