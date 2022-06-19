#include <ice/ui_element.hxx>
#include <ice/ui_data.hxx>
#include <ice/assert.hxx>

namespace ice::ui
{

    auto to_vec2(ice::ui::Position pos) noexcept -> ice::vec2u
    {
        return { static_cast<ice::u32>(pos.x), static_cast<ice::u32>(pos.y) };
    }

    auto move_box(
        ice::ui::Rect bbox,
        ice::vec2u offset
    ) noexcept
    {
        bbox.left += offset.x;
        bbox.right += offset.x;
        bbox.top += offset.y;
        bbox.bottom += offset.y;
        return bbox;
    }

    auto make_bbox(
        ice::ui::Size size,
        ice::ui::RectOffset margin,
        ice::ui::RectOffset padding
    ) noexcept -> ice::ui::Rect
    {
        ice::ui::Rect bbox{ };
        bbox.left = 0;
        bbox.top = 0;
        bbox.right = size.width + (margin.left + margin.right) + (padding.left + padding.right);
        bbox.bottom = size.height + (margin.top + margin.bottom) + (padding.top + padding.bottom);
        return bbox;
    }

    auto make_hitbox(
        ice::ui::Rect bbox,
        ice::ui::RectOffset margin
    ) noexcept -> ice::ui::Rect
    {
        return move_box(bbox - margin, to_vec2({ margin.left, margin.top }));
    }

    auto element_update_size_explicit(
        ice::ui::UIData const& data,
        ice::ui::Element const& parent,
        ice::ui::ElementInfo const& info,
        ice::ui::Element& out_element
    ) noexcept
    {
        Size size;
        Position position;
        RectOffset margin;
        RectOffset padding;
        ElementFlags flags;

        read_size(data, info, size, flags);
        read_margin(data, info, margin, flags);
        read_padding(data, info, padding, flags);

        out_element.bbox = make_bbox(size, margin, padding);
        out_element.hitbox = make_hitbox(out_element.bbox, margin);
    }

    auto element_update_size_auto(
        ice::ui::UIData const& data,
        ice::ui::Element const& parent,
        ice::ui::ElementInfo const& info,
        ice::ui::Element& out_element
    ) noexcept
    {
        Size size;
        RectOffset margin;
        RectOffset padding;
        ElementFlags flags;

        read_size(data, info, size, flags);
        read_margin(data, info, margin, flags);
        read_padding(data, info, padding, flags);

        if (contains(flags, ElementFlags::Size_AutoWidth))
        {
            // TOOD: Calculate width depending on element type
            ICE_ASSERT(false, "");
            size.width = 0;
        }
        if (contains(flags, ElementFlags::Size_AutoHeight))
        {
            // TOOD: Calculate height depending on element type
            ICE_ASSERT(false, "");
            size.height = 0;
        }

        out_element.bbox = make_bbox(size, margin, padding);
        out_element.hitbox = make_hitbox(out_element.bbox, margin);
    }

    auto element_update_size_stretch(
        ice::ui::UIData const& data,
        ice::ui::Element const& parent,
        ice::ui::ElementInfo const& info,
        ice::ui::Element& out_element
    ) noexcept
    {
        Size size;
        RectOffset margin;
        RectOffset padding;
        ElementFlags flags;

        read_size(data, info, size, flags);
        read_margin(data, info, margin, flags);
        read_padding(data, info, padding, flags);

        // We use the hitbox, as we cannot outgrow the 'margin' value of the parent element.
        ice::ui::Size const parent_size = rect_size(parent.hitbox);
        ice::ui::Size const available_size = rect_size(make_bbox(size, -margin, -padding));

        if (contains(flags, ElementFlags::Size_StretchWidth))
        {
            size.width = available_size.width;
        }
        if (contains(flags, ElementFlags::Size_StretchHeight))
        {
            size.height = available_size.height;
        }

        out_element.bbox = make_bbox(size, margin, padding);
        out_element.hitbox = make_hitbox(out_element.bbox, margin);
    }

    auto element_update_position(
        ice::ui::UIData const& data,
        ice::ui::Element const& parent,
        ice::ui::ElementInfo const& info,
        ice::ui::Element& out_element
    ) noexcept
    {
        Position position;
        RectOffset margin;
        RectOffset padding;
        ElementFlags flags;

        read_position(data, info, position, flags);

        // We use the hitbox, as we should start at the 'padded' location of the parent element.
        ice::ui::Position const parent_pos = rect_position(parent.hitbox);

        // TODO: Take into consideration anhor flags

        out_element.bbox = move_box(out_element.bbox, to_vec2(parent_pos));
        out_element.hitbox = move_box(out_element.hitbox, to_vec2(parent_pos));
    }

    auto element_update(
        ice::ui::UpdateStage stage,
        ice::ui::UIData const& data,
        ice::ui::Element const& parent,
        ice::ui::ElementInfo const& info,
        ice::ui::Element& out_element
    ) noexcept
    {
        //ElementType const parent_type = data.elements[info.parent].type;
        if (stage == UpdateStage::ExplicitSize)
        {
            element_update_size_explicit(data, parent, info, out_element);
        }
        else if (stage == UpdateStage::AutoSize)
        {
            element_update_size_auto(data, parent, info, out_element);
        }
        else if (stage == UpdateStage::StretchSize)
        {
            element_update_size_stretch(data, parent, info, out_element);
        }
    }

} // namespace ice
