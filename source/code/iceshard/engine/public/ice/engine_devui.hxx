#pragma once
#include <ice/base.hxx>

namespace ice
{

    namespace devui
    {

        class DevUIWidget;
        class DevUITrait;

        enum class DevUIExecutionKey : ice::u32;

    } // namespace devui

    class EngineFrame;

    class EngineDevUI
    {
    public:
        virtual ~EngineDevUI() noexcept = default;

        virtual auto world_trait() noexcept -> ice::devui::DevUITrait* = 0;

        virtual void register_widget(ice::devui::DevUIWidget* widget) noexcept = 0;
        virtual void unregister_widget(ice::devui::DevUIWidget* widget) noexcept = 0;

    public:
        virtual void internal_set_key(devui::DevUIExecutionKey) noexcept = 0;
        //virtual void internal_prepared_widgets(devui::DevUIExecutionKey) noexcept = 0;
        virtual void internal_build_widgets(
            ice::EngineFrame& frame,
            ice::devui::DevUIExecutionKey
        ) noexcept = 0;
    };

} // namespace ice
