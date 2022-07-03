#include <ice/ui_element_info.hxx>
#include <ice/ui_data.hxx>
#include <ice/assert.hxx>

namespace ice::ui
{

    void read_size(
        ice::ui::UIData const& uidata,
        ice::ui::ElementInfo const& info,
        ice::ui::Size& out_size,
        ice::ui::ElementFlags& out_flags
    ) noexcept
    {
        ice::u16 const index = info.size_i & 0x0fff;
        ice::u32 const flag_values = (info.size_i & 0xf000) >> 12;

        ICE_ASSERT(
            index < ice::size(uidata.sizes),
            "Trying to read 'size' value for element index outside of the given data. [ idx:{} | range:0 .. {}]",
            info.size_i,
            ice::size(uidata.sizes)
        );

        out_flags = out_flags | static_cast<ice::ui::ElementFlags>(flag_values << 0);
        out_size = uidata.sizes[index];
    }

    void read_position(
        ice::ui::UIData const& uidata,
        ice::ui::ElementInfo const& info,
        ice::ui::Position& out_position,
        ice::ui::ElementFlags& out_flags
    ) noexcept
    {
        ice::u16 const index = info.pos_i & 0x0fff;
        ice::u32 const flag_values = (info.pos_i & 0xf000) >> 12;

        ICE_ASSERT(
            index < ice::size(uidata.positions),
            "Trying to read 'position' value for element index outside of the given data. [ idx:{} | range:0 .. {}]",
            info.pos_i,
            ice::size(uidata.positions)
        );

        out_flags = out_flags | static_cast<ice::ui::ElementFlags>(flag_values << 4);
        out_position = uidata.positions[index];
    }

    void read_margin(
        ice::ui::UIData const& uidata,
        ice::ui::ElementInfo const& info,
        ice::ui::RectOffset& out_rect_offset,
        ice::ui::ElementFlags& out_flags
    ) noexcept
    {
        ice::u16 const index = info.mar_i & 0x0fff;
        ice::u32 const flag_values = (info.mar_i & 0xf000) >> 12;

        ICE_ASSERT(
            index < ice::size(uidata.margins),
            "Trying to read 'margin' value for element index outside of the given data. [ idx:{} | range:0 .. {}]",
            info.mar_i,
            ice::size(uidata.margins)
        );

        out_flags = out_flags | static_cast<ice::ui::ElementFlags>(flag_values << 12);
        out_rect_offset = uidata.margins[index];
    }

    void read_padding(
        ice::ui::UIData const& uidata,
        ice::ui::ElementInfo const& info,
        ice::ui::RectOffset& out_rect_offset,
        ice::ui::ElementFlags& out_flags
    ) noexcept
    {
        ice::u16 const index = info.mar_i & 0x0fff;
        ice::u32 const flag_values = (info.mar_i & 0xf000) >> 12;

        ICE_ASSERT(
            index < ice::size(uidata.paddings),
            "Trying to read 'padding' value for element index outside of the given data. [ idx:{} | range:0 .. {}]",
            info.pad_i,
            ice::size(uidata.paddings)
        );

        out_flags = out_flags | static_cast<ice::ui::ElementFlags>(flag_values << 12);
        out_rect_offset = uidata.paddings[index];
    }

} // namespace ice::ui
