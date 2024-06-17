#pragma once
#include <ice/devui_types.hxx>

namespace ice
{

    struct DevUIWidgetState
    {
        bool active = false;
    };

    class DevUIFrame
    {
    public:
        virtual ~DevUIFrame() noexcept = default;

        virtual void mainmenu(ice::DevUIWidgetInfo const& widget, ice::DevUIWidgetState& state) noexcept = 0;

        virtual bool begin(ice::DevUIWidgetInfo const& widget, ice::DevUIWidgetState& state) noexcept = 0;
        virtual void end() noexcept = 0;
    };

} // namespace ice
