#pragma once
#include <ice/ui_types.hxx>
#include <ice/ui_element_info.hxx>
#include <ice/ui_element_draw.hxx>

namespace ice::ui
{

    struct Element
    {
        ice::ui::ElementInfo const* definition;

        ice::ui::Rect bbox;
        ice::ui::Rect hitbox;
        ice::ui::DrawData draw_data;

        bool center_vertical : 1;
        bool center_horizontal : 1;
    };

    enum class UpdateStage : ice::u8
    {
        ExplicitSize,
        AutoSize,
        StretchSize,
        Position,
    };

    auto element_update(
        ice::ui::UpdateStage stage,
        ice::ui::UIData const& data,
        ice::ui::Element const& parent,
        ice::ui::ElementInfo const& info,
        ice::ui::Element& out_element
    ) noexcept;

    static constexpr ice::ui::UpdateStage Constant_UpdateStageOrder[]{
        UpdateStage::ExplicitSize,
        UpdateStage::AutoSize,
        UpdateStage::StretchSize,
        UpdateStage::Position
    };

} // namespace ice::ui
