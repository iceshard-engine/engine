#pragma once
#include <ice/ui_types.hxx>

namespace ice::ui
{

    enum class ElementType : ice::u8
    {
        Page,
        ListBox,
        Button,
        Label,
        Custom0,
    };

    struct ElementInfo
    {
        // first 4 bits (four bits of locking)
        ice::u16 parent;

        // 12bits == 4095 possible elements
        ice::u16 size_i; // first four bits == auto + stretch values
        ice::u16 pos_i; // first four bits == auto + {???} values
        ice::u16 mar_i; // first four bits == auto values
        ice::u16 pad_i; // first four bits == auto values

        ice::ui::ElementType type;
        ice::u8 type_data_i;
    };

    void read_size(
        ice::ui::UIData const& uidata,
        ice::ui::ElementInfo const& info,
        ice::ui::Size& out_size,
        ice::ui::ElementFlags& out_flags
    ) noexcept;

    void read_position(
        ice::ui::UIData const& uidata,
        ice::ui::ElementInfo const& info,
        ice::ui::Position& out_position,
        ice::ui::ElementFlags& out_flags
    ) noexcept;

    void read_margin(
        ice::ui::UIData const& uidata,
        ice::ui::ElementInfo const& info,
        ice::ui::RectOffset& out_rect_offset,
        ice::ui::ElementFlags& out_flags
    ) noexcept;

    void read_padding(
        ice::ui::UIData const& uidata,
        ice::ui::ElementInfo const& info,
        ice::ui::RectOffset& out_rect_offset,
        ice::ui::ElementFlags& out_flags
    ) noexcept;


    enum class ElementFlags : ice::u16
    {
        None = 0x0,

        Size_AutoWidth = 0x0000'0008,
        Size_AutoHeight = 0x0000'0004,
        Size_StretchWidth = 0x0000'0002,
        Size_StretchHeight = 0x0000'0001,

        Position_AnchorLeft = 0x0000'0080,
        Position_AnchorRight = 0x0000'0040,
        Position_AnchorTop = 0x0000'0020,
        Position_AnchorBottom = 0x0000'0010,

        Offset_AutoLeft = 0x0000'0800,
        Offset_AutoTop = 0x0000'0400,
        Offset_AutoRight = 0x0000'0200,
        Offset_AutoBottom = 0x0000'0100,
    };

    constexpr auto operator|(
        ice::ui::ElementFlags left,
        ice::ui::ElementFlags right
    ) noexcept -> ice::ui::ElementFlags
    {
        ice::u32 const left_value = static_cast<ice::u32>(left);
        ice::u32 const right_value = static_cast<ice::u32>(right);
        return static_cast<ice::ui::ElementFlags>(left_value | right_value);
    }

    constexpr auto operator&(
        ice::ui::ElementFlags left,
        ice::ui::ElementFlags right
    ) noexcept -> ice::ui::ElementFlags
    {
        ice::u32 const left_value = static_cast<ice::u32>(left);
        ice::u32 const right_value = static_cast<ice::u32>(right);
        return static_cast<ice::ui::ElementFlags>(left_value & right_value);
    }

    constexpr bool contains(
        ice::ui::ElementFlags searched_value,
        ice::ui::ElementFlags searched_flags
    ) noexcept
    {
        return (searched_value & searched_flags) == searched_flags;
    }

} // namespace ice::ui
