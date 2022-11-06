/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/devui/devui_widget.hxx>

#include "trait_box2d.hxx"
#include "box2d.hxx"

namespace ice
{

    class DevUI_Box2D : public ice::devui::DevUIWidget
    {
    public:
        DevUI_Box2D(
            b2World& box2d_world
        ) noexcept;

        auto settings() const noexcept -> ice::devui::WidgetSettings const& override;

        void on_prepare(void* context, ice::devui::WidgetState& state) noexcept override;
        void on_draw() noexcept override;

        void on_frame(ice::EngineFrame& frame) noexcept;

    private:
        b2World& _world;
        ice::devui::WidgetState* _state;
        ice::u32 _debug_draw_flags;
    };

} // namespace ice
