#include <ice/ui_button.hxx>
#include <ice/ui_action.hxx>
#include <ice/ui_data.hxx>
#include <ice/memory/pointer_arithmetic.hxx>

namespace ice::ui
{

    auto button_get_text(
        ice::ui::UIData const& data,
        ice::ui::ButtonInfo const& button_info,
        ice::Span<ice::ui::UIResourceData const> resources
    ) noexcept -> ice::Utf8String
    {
        ice::Utf8String text;
        if (button_info.action_text_i != ice::u16{ 0xffff })
        {
            Action const& action = data.ui_actions[button_info.action_text_i];
            if (action.type == ActionType::Shard && action.type_data == ActionData::ValueResource)
            {
                UIResourceData const& data = resources[action.type_data_i];
                if (data.info.type == ResourceType::Utf8String)
                {
                    text = *reinterpret_cast<ice::Utf8String const*>(data.location);
                }
            }
            else if (action.type == ActionType::Data)
            {
                UIResourceData const& data = resources[action.type_data_i];
                if (data.info.type == ResourceType::Utf8String)
                {
                    text = *reinterpret_cast<ice::Utf8String const*>(data.location);
                }
            }
        }
        else
        {
            text = {
                reinterpret_cast<ice::c8utf const*>(ice::memory::ptr_add(data.additional_data, button_info.text_offset)),
                button_info.text_size
            };
        }
        return text;
    }

} // namespace ice::ui
