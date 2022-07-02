#pragma once
#include <ice/ui_types.hxx>
#include <ice/string.hxx>
#include <ice/span.hxx>

namespace ice::ui
{

    struct ButtonInfo
    {
        ice::usize text_offset;
        ice::u32 text_size;

        ice::u16 action_text_i;
        ice::u16 action_on_click_i;
    };

    auto button_get_text(
        ice::ui::UIData const& data,
        ice::ui::ButtonInfo const& button_info,
        ice::Span<ice::ui::UIResourceData const> resources
    ) noexcept ->ice::Utf8String;

} // namespace ice::ui
