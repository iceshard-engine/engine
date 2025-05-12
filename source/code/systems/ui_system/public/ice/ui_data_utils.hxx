/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/ui_page.hxx>
#include <ice/ui_data_ref.hxx>
#include <ice/ui_resource.hxx>
#include <ice/ui_style.hxx>
#include <ice/ui_font.hxx>

#include <ice/assert.hxx>
#include <ice/span.hxx>

namespace ice::ui
{

    auto element_get_text(
        ice::ui::PageInfo const& data,
        ice::ui::ElementWithText auto const& element,
        ice::Span<ice::ui::UIResourceData const> resources
    ) noexcept -> ice::String
    {
        ice::String text{ };
        if (element.text.source == DataSource::ValueConstant)
        {
            ice::ui::ConstantInfo const& constant = data.ui_constants[element.text.source_i];
            text = {
                reinterpret_cast<char const*>(ice::ptr_add(data.additional_data, { constant.offset })),
                constant.size
            };
        }
        else if (element.text.source == DataSource::ValueResource)
        {
            ice::ui::UIResourceData const& resource = resources[element.text.source_i];
            if (resource.info.type == ResourceType::String)
            {
                text = *reinterpret_cast<ice::String const*>(resource.location);
            }
        }
        return text;
    }

    inline bool element_get_style(
        ice::ui::PageInfo const& data,
        ice::ui::Element const& element,
        ice::ui::StyleFlags style_target,
        ice::ui::StyleColor const*& out_color
    ) noexcept
    {
        ice::ui::ElementInfo const& element_info = *element.definition;
        if (element_info.style_i == 0)
        {
            return false;
        }

        ice::u32 match_value = 0;
        ice::ui::StyleInfo const* matched_style = &data.styles[element_info.style_i];
        for (ice::ui::StyleInfo const& style_info : data.styles)
        {
            if (style_info.target_state != ElementState::Any)
            {
                ice::u32 match = 0;
                if (has_all(element.state, style_info.target_state))
                {
                    match = 2;
                }
                else if (has_any(element.state, style_info.target_state))
                {
                    match = 1;
                }

                if (match > match_value)
                {
                    match_value = match;
                    matched_style = &style_info;
                }
            }
        }

        bool found = false;
        if (has_all(matched_style->flags, style_target & StyleFlags::BackgroundColor))
        {
            found = true;
            ICE_ASSERT(matched_style->data_bg.source == DataSource::ValueConstant, "Colors cannot be defined outside of constants for now!");
            ice::ui::ConstantInfo const& constant_loc = data.ui_constants[matched_style->data_bg.source_i];

            void const* color_data = ice::ptr_add(data.additional_data, { constant_loc.offset });
            out_color = reinterpret_cast<ice::ui::StyleColor const*>(color_data);
        }
        return found;
    }

    auto element_get_font(
        ice::ui::PageInfo const& data,
        ice::ui::ElementWithFont auto const& element,
        ice::Span<ice::ui::UIResourceData const> resources
    ) noexcept -> ice::Font const*
    {
        ice::Font const* result = nullptr;
        if (element.font.source == DataSource::ValueResource)
        {
            ice::ui::FontInfo const& font_info = data.fonts[element.font.source_i];
            ice::ui::UIResourceData const& res_data = resources[font_info.resource_i];
            if (res_data.info.type == ResourceType::Font)
            {
                result = *reinterpret_cast<ice::Font const**>(res_data.location);
            }
        }
        return result;
    }

} // namespace ice::ui
