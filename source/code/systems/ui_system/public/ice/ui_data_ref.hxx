#pragma once
#include <ice/base.hxx>

namespace ice::ui
{

    enum class DataSource : ice::u16
    {
        None,
        ValueConstant,
        ValueResource,
        ValueProperty,
    };

    struct DataRef
    {
        ice::ui::DataSource source;
        ice::u16 source_i;
    };

    static_assert(sizeof(DataRef) == 4);


    struct ConstantInfo
    {
        ice::u32 offset;
        ice::u32 size;
    };


    template<typename T>
    concept ElementWithText = requires(T t)
    {
        { t.text } -> std::convertible_to<ice::ui::DataRef>;
    };

    template<typename T>
    concept ElementWithFont = requires(T t)
    {
        { t.font } -> std::convertible_to<ice::ui::DataRef>;
    };

} // namespace ice::ui
