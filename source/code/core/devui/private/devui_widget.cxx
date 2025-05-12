/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/devui_widget.hxx>
#include <ice/devui_frame.hxx>
#include <ice/devui_imgui.hxx>

namespace ice
{

    DevUIWidget::DevUIWidget(ice::DevUIWidgetInfo const& info) noexcept
        : info{ info }
    {
    }

    void DevUIWidget::build_widget(ice::DevUIFrame& frame, ice::DevUIWidgetState& state) noexcept
    {
        if (frame.begin(info, state))
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
