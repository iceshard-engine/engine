/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include "webasm_core_app.hxx"

namespace ice::platform::webasm
{

    class WebAsmApp : public WebAsmCoreApp
    {
    public:
        WebAsmApp() noexcept;
        ~WebAsmApp() noexcept;

    public: // Impl: ice::platform::Core
        auto refresh_events() noexcept -> ice::Result override;

    public: // Native input callbacks
        void on_mouse_event(int type, EmscriptenMouseEvent const& event) noexcept;

    private:
        pthread_mutex_t _mutex;
        ice::input::DeviceEventQueue _temporary_inputs;
    };

} // namespace ice::platform::webasm
