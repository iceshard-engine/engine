#pragma once
#include <ice/ui_types.hxx>

namespace ice::ui
{

    enum class ElementType : ice::u8
    {
        Page,
        VListBox,
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

        Size_AutoWidth = 0x0001,
        Size_AutoHeight = 0x0002,
        Size_StretchWidth = 0x0004,
        Size_StretchHeight = 0x0008,

        Position_AutoX = 0x00010,
        Position_AutoY = 0x00020,
        Position_AnchorLeft = 0x0100,
        Position_AnchorRight = 0x0200,
        Position_AnchorTop = 0x0400,
        Position_AnchorBottom = 0x0800,

        Offset_AutoLeft = 0x1000,
        Offset_AutoTop = 0x2000,
        Offset_AutoRight = 0x4000,
        Offset_AutoBottom = 0x8000,
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

    constexpr auto operator~(
        ice::ui::ElementFlags flags
    ) noexcept -> ice::ui::ElementFlags
    {
        ElementFlags constexpr AllValidFlags = ElementFlags::None
            | ElementFlags::Size_AutoWidth | ElementFlags::Size_AutoHeight
            | ElementFlags::Size_StretchWidth | ElementFlags::Size_StretchHeight
            | ElementFlags::Position_AutoX | ElementFlags::Position_AutoY
            | ElementFlags::Position_AnchorLeft | ElementFlags::Position_AnchorRight
            | ElementFlags::Position_AnchorTop | ElementFlags::Position_AnchorBottom
            | ElementFlags::Offset_AutoLeft | ElementFlags::Offset_AutoRight
            | ElementFlags::Offset_AutoTop | ElementFlags::Offset_AutoBottom;

        ice::u32 const flags_value = static_cast<ice::u32>(flags);
        ice::u32 const all_value = static_cast<ice::u32>(AllValidFlags);
        return static_cast<ice::ui::ElementFlags>(all_value ^ flags_value);
    }

    constexpr bool contains(
        ice::ui::ElementFlags searched_value,
        ice::ui::ElementFlags searched_flags
    ) noexcept
    {
        return (searched_value & searched_flags) == searched_flags;
    }

    constexpr bool any(
        ice::ui::ElementFlags searched_value,
        ice::ui::ElementFlags searched_flags
    ) noexcept
    {
        return (searched_value & searched_flags) != ice::ui::ElementFlags::None;
    }

} // namespace ice::ui
