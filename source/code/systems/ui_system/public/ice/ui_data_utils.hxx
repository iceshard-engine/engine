#pragma once
#include <ice/ui_page.hxx>
#include <ice/ui_data_ref.hxx>
#include <ice/ui_resource.hxx>

#include <ice/memory/pointer_arithmetic.hxx>
#include <ice/span.hxx>

namespace ice::ui
{

    auto element_get_text(
        ice::ui::PageInfo const& data,
        ice::ui::ElementWithText auto const& element,
        ice::Span<ice::ui::UIResourceData const> resources
    ) noexcept -> ice::Utf8String
    {
        ice::Utf8String text{ };
        if (element.text.source == DataSource::ValueConstant)
        {
            ice::ui::ConstantInfo const& constant = data.ui_constants[element.text.source_i];
            text = {
                reinterpret_cast<ice::c8utf const*>(ice::memory::ptr_add(data.additional_data, constant.offset)),
                constant.size
            };
        }
        else if (element.text.source == DataSource::ValueResource)
        {
            ice::ui::UIResourceData const& resource = resources[element.text.source_i];
            if (resource.info.type == ResourceType::Utf8String)
            {
                text = *reinterpret_cast<ice::Utf8String const*>(resource.location);
            }
        }
        return text;
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
