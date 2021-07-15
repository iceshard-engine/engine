#pragma once
#include <ice/stringid.hxx>
#include <ice/engine_devui.hxx>

namespace ice::devui
{

    class DevUIWidget;

    class DevUISystem : public ice::EngineDevUI
    {
    public:
        virtual ~DevUISystem() noexcept = default;

        //virtual void begin_context() noexcept = 0;
        //virtual void end_context() noexcept = 0;
    };

} // namespace ice::devui
