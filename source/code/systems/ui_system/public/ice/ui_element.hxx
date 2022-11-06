/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

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
        ice::ui::Rect contentbox;
        ice::ui::DrawData draw_data;
        ice::ui::ElementFlags flags;
        ice::ui::ElementState state;

        ice::ui::Element* child;
        ice::ui::Element* sibling;
    };

    enum class UpdateStage : ice::u8
    {
        ExplicitSize,
        AutoSize,
        StretchSize,
        Position,
    };

    enum class UpdateResult : ice::u8
    {
        Unresolved,
        Resolved,
    };

    auto element_update(
        ice::ui::UpdateStage stage,
        ice::ui::PageInfo const& data,
        ice::ui::Element const& parent,
        ice::ui::ElementInfo const& info,
        ice::ui::Element& out_element,
        ice::Span<ice::ui::UIResourceData const> resources
    ) noexcept -> ice::ui::UpdateResult;

    static constexpr ice::ui::UpdateStage Constant_UpdateStageOrder[]{
        UpdateStage::ExplicitSize,
        UpdateStage::AutoSize,
        UpdateStage::StretchSize,
        UpdateStage::Position
    };

} // namespace ice::ui
