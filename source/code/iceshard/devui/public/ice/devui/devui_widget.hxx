#pragma once

namespace ice::devui
{

    class DevUIWidget
    {
    public:
        virtual ~DevUIWidget() noexcept = default;

        virtual void on_prepare(void* context) noexcept { }

        virtual void on_draw() noexcept = 0;
    };

} // namespace ice::devui
