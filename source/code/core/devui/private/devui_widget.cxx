/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/devui_widget.hxx>
#include <ice/devui_frame.hxx>
#include <ice/devui_imgui.hxx>

namespace ice
{

    DevUIWidget::DevUIWidget(ice::DevUIWidgetInfo const& info) noexcept
        : IDevUIWidget{ }
        , _widget_info{ info }
    {
    }

    void DevUIWidget::build_widget(ice::DevUIFrame& frame, ice::DevUIWidgetState& state) noexcept
    {
        if (frame.begin(*this, state))
        {
            this->build_menu();
            this->build_content();
        }
        frame.end();
    }

    void DevUIWidget::build_menu() noexcept
    {
    }

    bool DevUIWidget::build_mainmenu(ice::DevUIWidgetState& state) noexcept
    {
        return true;
    }

} // namespace ice
