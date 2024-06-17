#pragma once
#include <ice/devui_types.hxx>
#include <ice/string/string.hxx>

namespace ice
{

    struct DevUIWidgetInfo
    {
        ice::String category;
        ice::String name;
    };

    class DevUIWidget
    {
    public:
        DevUIWidget(ice::DevUIWidgetInfo const& info) noexcept;
        virtual ~DevUIWidget() noexcept = default;

        virtual void build_content() noexcept = 0;

        virtual void build_widget(ice::DevUIFrame& frame, ice::DevUIWidgetState& state) noexcept;

        virtual void build_menu() noexcept;

        virtual bool build_mainmenu(ice::DevUIWidgetState& state) noexcept;

        ice::DevUIWidgetInfo const info;
    };

} // namespace ice
