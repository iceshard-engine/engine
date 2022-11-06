/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/ui_element_info.hxx>
#include <ice/ui_page.hxx>
#include <ice/assert.hxx>

namespace ice::ui
{

    void read_size(
        ice::ui::PageInfo const& uidata,
        ice::ui::ElementInfo const& info,
        ice::ui::Size& out_size
    ) noexcept
    {
        ice::u16 const index = info.size_i & 0x0fff;

        ICE_ASSERT(
            index < ice::count(uidata.sizes),
            "Trying to read 'size' value for element index outside of the given data. [ idx:{} | range:0 .. {}]",
            info.size_i,
            ice::count(uidata.sizes)
        );

        out_size = uidata.sizes[index];
    }

    void read_position(
        ice::ui::PageInfo const& uidata,
        ice::ui::ElementInfo const& info,
        ice::ui::Position& out_position
    ) noexcept
    {
        ice::u16 const index = info.pos_i & 0x0fff;

        ICE_ASSERT(
            index < ice::count(uidata.positions),
            "Trying to read 'position' value for element index outside of the given data. [ idx:{} | range:0 .. {}]",
            info.pos_i,
            ice::count(uidata.positions)
        );

        out_position = uidata.positions[index];
    }

    void read_margin(
        ice::ui::PageInfo const& uidata,
        ice::ui::ElementInfo const& info,
        ice::ui::RectOffset& out_rect_offset
    ) noexcept
    {
        ice::u16 const index = info.mar_i & 0x0fff;

        ICE_ASSERT(
            index < ice::count(uidata.margins),
            "Trying to read 'margin' value for element index outside of the given data. [ idx:{} | range:0 .. {}]",
            info.mar_i,
            ice::count(uidata.margins)
        );

        out_rect_offset = uidata.margins[index];
    }

    void read_padding(
        ice::ui::PageInfo const& uidata,
        ice::ui::ElementInfo const& info,
        ice::ui::RectOffset& out_rect_offset
    ) noexcept
    {
        ice::u16 const index = info.mar_i & 0x0fff;

        ICE_ASSERT(
            index < ice::count(uidata.paddings),
            "Trying to read 'padding' value for element index outside of the given data. [ idx:{} | range:0 .. {}]",
            info.pad_i,
            ice::count(uidata.paddings)
        );

        out_rect_offset = uidata.paddings[index];
    }

} // namespace ice::ui
