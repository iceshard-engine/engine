/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/devui/devui_widget.hxx>

#include "trait_chipmunk2d.hxx"
#include "chipmunk2d.hxx"

namespace ice
{

    class DevUI_Chipmunk2D : public ice::devui::DevUIWidget
    {
    public:
        DevUI_Chipmunk2D(
            cpSpace& space
        ) noexcept;

        auto settings() const noexcept -> ice::devui::WidgetSettings const& override;

        void on_prepare(void* context, ice::devui::WidgetState& state) noexcept override;
        void on_draw() noexcept override;

        void on_frame(ice::EngineFrame& frame) noexcept;

    private:
        cpSpace& _space;
        ice::devui::WidgetState* _state;
        ice::u32 _debug_draw_flags;
    };

} // namespace ice
