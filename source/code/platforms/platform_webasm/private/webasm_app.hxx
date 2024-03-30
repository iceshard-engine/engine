/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include "webasm_core_app.hxx"
#include <ice/container/queue.hxx>

namespace ice::platform::webasm
{

    struct WebAsmTextEvent
    {
        char input[32];
    };

    class WebAsmApp : public WebAsmCoreApp
    {
    public:
        static WebAsmApp* global_instance;

        WebAsmApp() noexcept;
        ~WebAsmApp() noexcept;

        void initialize(ice::Span<ice::Shard const> shards) noexcept override;

    public: // Impl: ice::platform::Core
        auto refresh_events() noexcept -> ice::Result override;

    public: // Native input callbacks
        void on_mouse_event(int type, EmscriptenMouseEvent const& event) noexcept;
        bool on_key_event(int type, EmscriptenKeyboardEvent const& event) noexcept;

    private:
        pthread_mutex_t _mutex;
        ice::input::DeviceEventQueue _temporary_inputs;

        ice::Queue<WebAsmTextEvent> _text_events, _temp_text_events;
    };

} // namespace ice::platform::webasm
