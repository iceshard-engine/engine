/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
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
            index < uidata.sizes.size(),
            "Trying to read 'size' value for element index outside of the given data. [ idx:{} | range:0 .. {}]",
            info.size_i,
            uidata.sizes.size()
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
            index < uidata.positions.size(),
            "Trying to read 'position' value for element index outside of the given data. [ idx:{} | range:0 .. {}]",
            info.pos_i,
            uidata.positions.size()
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
            index < uidata.margins.size(),
            "Trying to read 'margin' value for element index outside of the given data. [ idx:{} | range:0 .. {}]",
            info.mar_i,
            uidata.margins.size()
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
            index < uidata.paddings.size(),
            "Trying to read 'padding' value for element index outside of the given data. [ idx:{} | range:0 .. {}]",
            info.pad_i,
            uidata.paddings.size()
        );

        out_rect_offset = uidata.paddings[index];
    }

} // namespace ice::ui
