#pragma once
#include <ice/ui_types.hxx>

namespace ice::ui
{

    enum class ElementType : ice::u8
    {
        Page,
        LayoutV,
        LayoutH,
        Label,
        Button,
        Custom0 = 128,
    };

    struct ElementInfo
    {
        ice::u16 parent;

        ice::u16 style_i;
        ice::u16 size_i;
        ice::u16 pos_i;
        ice::u16 mar_i;
        ice::u16 pad_i;

        ice::ui::ElementType type;
        ice::u8 type_data_i;

        ice::ui::ElementFlags flags;
    };

    static_assert(sizeof(ElementInfo) == 20);

    enum class ElementFlags : ice::u32
    {
        None = 0x0,

        Size_AutoWidth = 0x0001,
        Size_AutoHeight = 0x0002,
        Size_StretchWidth = 0x0004,
        Size_StretchHeight = 0x0008,

        Position_AutoX = 0x0010,
        Position_AutoY = 0x0020,
        Position_PercentageX = 0x0040,
        Position_PercentageY = 0x0080,

        Offset_AutoLeft = 0x1000,
        Offset_AutoTop = 0x2000,
        Offset_AutoRight = 0x4000,
        Offset_AutoBottom = 0x8000,

        All = Size_AutoWidth | Size_AutoHeight
            | Size_StretchWidth | Size_StretchHeight
            | Position_AutoX | Position_AutoY
            | Position_PercentageX | Position_PercentageY
            | Offset_AutoLeft | Offset_AutoRight
            | Offset_AutoTop | Offset_AutoBottom
    };


    void read_size(
        ice::ui::PageInfo const& uidata,
        ice::ui::ElementInfo const& info,
        ice::ui::Size& out_size
    ) noexcept;

    void read_position(
        ice::ui::PageInfo const& uidata,
        ice::ui::ElementInfo const& info,
        ice::ui::Position& out_position
    ) noexcept;

    void read_margin(
        ice::ui::PageInfo const& uidata,
        ice::ui::ElementInfo const& info,
        ice::ui::RectOffset& out_rect_offset
    ) noexcept;

    void read_padding(
        ice::ui::PageInfo const& uidata,
        ice::ui::ElementInfo const& info,
        ice::ui::RectOffset& out_rect_offset
    ) noexcept;

} // namespace ice::ui
