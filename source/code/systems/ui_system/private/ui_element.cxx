#include <ice/ui_element.hxx>
#include <ice/ui_button.hxx>
#include <ice/ui_data.hxx>
#include <ice/font_utils.hxx>
#include <ice/memory/pointer_arithmetic.hxx>
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

    //auto make_hitbox(
    //    ice::ui::Rect bbox,
    //    ice::ui::RectOffset margin
    //) noexcept -> ice::ui::Rect
    //{
    //    return move_box(bbox - margin, to_vec2({ margin.left, margin.top }));
    //}

    //auto make_contentbox(
    //    ice::ui::Rect hitbox,
    //    ice::ui::RectOffset padding
    //) noexcept -> ice::ui::Rect
    //{
    //    return move_box(hitbox - padding, to_vec2({ padding.left, padding.top }));
    //}

    auto element_update_size_explicit(
        ice::ui::UIData const& data,
        ice::ui::Element const& parent,
        ice::ui::ElementInfo const& info,
        ice::ui::Element& out_element
    ) noexcept -> ice::ui::UpdateResult
    {
        Size size;
        Position position;
        RectOffset margin;
        RectOffset padding;
        ElementFlags flags = ElementFlags::None;

        read_size(data, info, size, flags);
        read_margin(data, info, margin, flags);
        read_padding(data, info, padding, flags);

        out_element.bbox = make_bbox(size, margin, padding);
        out_element.hitbox = out_element.bbox - margin;// make_hitbox(out_element.bbox, margin);
        out_element.contentbox = out_element.hitbox - padding;
        out_element.flags = flags;

        return any(
            flags,
            ElementFlags::Size_AutoWidth
            | ElementFlags::Size_AutoHeight
            | ElementFlags::Size_StretchWidth
            | ElementFlags::Size_StretchHeight
        ) ? UpdateResult::Unresolved : UpdateResult::Resolved;
    }

    auto element_update_size_auto(
        ice::ui::UIData const& data,
        ice::ui::Element const& parent,
        ice::ui::ElementInfo const& info,
        ice::ui::Element& out_element,
        ice::Span<ice::ui::UIResourceData const> resources
    ) noexcept -> ice::ui::UpdateResult
    {
        Size size;
        RectOffset margin;
        RectOffset padding;
        ElementFlags flags = ElementFlags::None;

        read_size(data, info, size, flags);
        read_margin(data, info, margin, flags);
        read_padding(data, info, padding, flags);

        ice::vec2f bounds{ 0.f };
        if (info.type == ElementType::Button)
        {
            ButtonInfo const& button_info = data.data_buttons[info.type_data_i];

            ice::Utf8String  const text = ice::ui::button_get_text(data, button_info, resources);

            ice::ui::FontInfo const& font_info = data.fonts[button_info.font_i];
            ice::Font const* font = ice::ui::button_get_font(data, button_info, resources);
            bounds = ice::font_text_bounds(*font, text);

            if (any(flags, ElementFlags::Size_AutoWidth | ElementFlags::Size_StretchWidth))
            {
                size.width = bounds.x * font_info.font_size;
            }
            if (any(flags, ElementFlags::Size_AutoHeight | ElementFlags::Size_StretchHeight))
            {
                size.height = bounds.y * font_info.font_size;
            }
        }
        else if (info.type == ElementType::VListBox)
        {
            Element* child = out_element.child;
            while (child != nullptr)
            {
                Size const child_size = rect_size(child->bbox);
                ElementFlags const child_flags = child->flags;

                if (any(flags, ElementFlags::Size_AutoWidth | ElementFlags::Size_StretchWidth))
                {
                    size.width = ice::max(size.width, child_size.width);
                }
                if (any(flags, ElementFlags::Size_AutoHeight | ElementFlags::Size_StretchHeight))
                {
                    size.height += child_size.height;
                }

                child = child->sibling;
            }
        }
        else
        {
            size = ice::ui::rect_size(out_element.contentbox);
        }

        out_element.bbox = make_bbox(size, margin, padding);
        out_element.hitbox = out_element.bbox - margin;// make_hitbox(out_element.bbox, margin);
        out_element.contentbox = out_element.hitbox - padding;
        out_element.flags = out_element.flags & ~(ElementFlags::Size_AutoWidth | ElementFlags::Size_AutoHeight);

        return any(
            flags,
            ElementFlags::Size_StretchWidth
            | ElementFlags::Size_StretchHeight
        ) ? UpdateResult::Unresolved : UpdateResult::Resolved;
    }

    auto element_update_size_stretch(
        ice::ui::UIData const& data,
        ice::ui::Element const& parent,
        ice::ui::ElementInfo const& info,
        ice::ui::Element& out_element
    ) noexcept -> ice::ui::UpdateResult
    {
        Size size;
        RectOffset margin;
        RectOffset padding;
        ElementFlags flags = ElementFlags::None;

        if (any(parent.flags, ElementFlags::Size_StretchWidth | ElementFlags::Size_StretchHeight))
        {
            return UpdateResult::Unresolved;
        }

        read_size(data, info, size, flags);
        if (any(flags, ElementFlags::Size_StretchWidth | ElementFlags::Size_StretchHeight) == false)
        {
            ICE_ASSERT(false, "Hmmm?!");
            return UpdateResult::Resolved;
        }

        read_margin(data, info, margin, flags);
        read_padding(data, info, padding, flags);

        size = ice::ui::rect_size(out_element.contentbox);

        // We use the hitbox, as we cannot outgrow the 'margin' value of the parent element.
        ice::ui::Size const parent_size = rect_size(parent.contentbox);
        if (contains(flags, ElementFlags::Size_StretchWidth))
        {
            size.width = parent_size.width - (padding.left + padding.right);
        }
        if (contains(flags, ElementFlags::Size_StretchHeight))
        {
            size.height = parent_size.height - (padding.top + padding.bottom);
        }

        out_element.bbox = make_bbox(size, margin, padding);
        out_element.hitbox = out_element.bbox - margin;// make_hitbox(out_element.bbox, margin);
        out_element.contentbox = out_element.hitbox - padding;
        out_element.flags = out_element.flags & ~(ElementFlags::Size_StretchWidth | ElementFlags::Size_StretchHeight);
        return UpdateResult::Resolved;
    }

    auto element_update_position(
        ice::ui::UIData const& data,
        ice::ui::Element const& parent,
        ice::ui::ElementInfo const& info,
        ice::ui::Element& out_element
    ) noexcept -> ice::ui::UpdateResult
    {
        Position position;
        ElementFlags current_flags = out_element.flags;
        read_position(data, info, position, current_flags);


        // We use the hitbox, as we should start at the 'padded' location of the parent element.
        Size const parent_size = rect_size(parent.hitbox);
        Size const size = rect_size(out_element.contentbox); // TOOD: Check if this should not be the hitbox.
        Position offset = rect_position(parent.hitbox);

        // If we are a child of a parent VListBox we are already updated
        if (parent.definition->type == ElementType::VListBox)
        {
            return UpdateResult::Resolved;
        }

        // Margin auto on left + right will center the page.
        if (contains(current_flags, ElementFlags::Offset_AutoLeft | ElementFlags::Offset_AutoRight))
        {
            Size const hitbox_size = rect_size(out_element.hitbox);
            ice::u32 const available_margin_width = ((parent_size.width - hitbox_size.width) / 2.f + 0.5f);

            offset.x = static_cast<ice::f32>(available_margin_width);
        }
        else if (contains(current_flags, ElementFlags::Position_AnchorRight))
        {
            ICE_ASSERT(
                contains(current_flags, ElementFlags::Position_AnchorRight) == false,
                "Cannot anchor on both left and right!"
            );

            offset.x += parent_size.width - (position.x + size.width);
        }
        else
        {
            offset.x += position.x;
        }

        if (contains(current_flags, ElementFlags::Offset_AutoTop | ElementFlags::Offset_AutoBottom))
        {
            Size const hitbox_size = rect_size(out_element.hitbox);
            ice::u32 const available_margin_height = ((parent_size.height - hitbox_size.height) / 2.f + 0.5f);

            offset.y = static_cast<ice::f32>(available_margin_height);
        }
        else if (contains(current_flags, ElementFlags::Position_AnchorBottom))
        {
            ICE_ASSERT(
                contains(current_flags, ElementFlags::Position_AnchorTop) == false,
                "Cannot anchor on both left and right!"
            );

            offset.y += parent_size.height - (position.y + size.height);
        }
        else
        {
            offset.y += position.y;
        }

        if (info.type == ElementType::VListBox)
        {
            ice::u32 offset_vertical = 0;

            Element* child = out_element.child;
            while (child != nullptr)
            {
                Size const child_size = rect_size(child->bbox);
                Position const child_offset = { offset.x, offset.y + offset_vertical };

                child->bbox = move_box(child->bbox, to_vec2(child_offset));
                child->hitbox = move_box(child->hitbox, to_vec2(child_offset));
                child->contentbox = move_box(child->contentbox, to_vec2(child_offset));
                offset_vertical += child_size.height + 0.5;

                child = child->sibling;
            }
        }

        out_element.bbox = move_box(out_element.bbox, to_vec2(offset));
        out_element.hitbox = move_box(out_element.hitbox, to_vec2(offset));
        out_element.contentbox = move_box(out_element.contentbox, to_vec2(offset));
        return UpdateResult::Resolved;
    }

    auto element_update(
        ice::ui::UpdateStage stage,
        ice::ui::UIData const& data,
        ice::ui::Element const& parent,
        ice::ui::ElementInfo const& info,
        ice::ui::Element& out_element,
        ice::Span<ice::ui::UIResourceData const> resources
    ) noexcept -> ice::ui::UpdateResult
    {
        //ElementType const parent_type = data.elements[info.parent].type;
        if (stage == UpdateStage::ExplicitSize)
        {
            return element_update_size_explicit(data, parent, info, out_element);
        }
        else if (stage == UpdateStage::AutoSize)
        {
            return element_update_size_auto(data, parent, info, out_element, resources);
        }
        else if (stage == UpdateStage::StretchSize)
        {
            return element_update_size_stretch(data, parent, info, out_element);
        }
        else if (stage == UpdateStage::Position)
        {
            return element_update_position(data, parent, info, out_element);
        }
    }

} // namespace ice
