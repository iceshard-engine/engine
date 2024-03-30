/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "webasm_app.hxx"
#include "webasm_inputs.hxx"
#include <ice/platform_event.hxx>

namespace ice::platform::webasm
{

    static constexpr ice::input::DeviceHandle Constant_Mouse = ice::input::make_device_handle(
        ice::input::DeviceType::Mouse, ice::input::DeviceIndex(0)
    );
    static constexpr ice::input::DeviceHandle Constant_Keyboard = ice::input::make_device_handle(
        ice::input::DeviceType::Keyboard, ice::input::DeviceIndex(0)
    );

    auto native_on_mouse_event(int type, EmscriptenMouseEvent const* event, void* userdata) -> EM_BOOL
    {
        WebAsmApp* app = reinterpret_cast<WebAsmApp*>(userdata);
        app->on_mouse_event(type, *event);
        return EM_TRUE;
    }

    auto native_on_keypress(int type, EmscriptenKeyboardEvent const* event, void* userdata) -> EM_BOOL
    {
        WebAsmApp* app = reinterpret_cast<WebAsmApp*>(userdata);
        return app->on_key_event(type, *event) ? EM_TRUE : EM_FALSE;
    }

    WebAsmApp* WebAsmApp::global_instance = nullptr;

    WebAsmApp::WebAsmApp() noexcept
        : WebAsmCoreApp{ }
        , _temporary_inputs{ _allocator }
        , _text_events{ _allocator }
        , _temp_text_events{ _allocator }
    {
        global_instance = this;

        _temporary_inputs.push(Constant_Mouse, ice::input::DeviceMessage::DeviceConnected);
        _temporary_inputs.push(Constant_Keyboard, ice::input::DeviceMessage::DeviceConnected);
        ice::queue::reserve(_text_events, 4);
        ice::queue::reserve(_temp_text_events, 4);
    }

    WebAsmApp::~WebAsmApp() noexcept
    {
        pthread_mutex_destroy(&_mutex);
    }

    void WebAsmApp::initialize(ice::Span<ice::Shard const> shards) noexcept
    {
        WebAsmCoreApp::initialize(shards);

        char const* input_selector_mouse = "#canvas";
        char const* input_selector_keyboard = EMSCRIPTEN_EVENT_TARGET_WINDOW;
        for (ice::Shard shard : shards)
        {
            if (shard == ShardID_WebAppInputSelectorMouse)
            {
                input_selector_mouse = ice::shard_shatter(shard, "#canvas");
            }
            if (shard == ShardID_WebAppInputSelectorKeyboard)
            {
                input_selector_keyboard = ice::shard_shatter(shard, EMSCRIPTEN_EVENT_TARGET_WINDOW);
            }
        }

        pthread_mutex_init(&_mutex, nullptr);
        // Mouse events
        emscripten_set_mousemove_callback(input_selector_mouse, this, EM_TRUE, &native_on_mouse_event);
        emscripten_set_mousedown_callback(input_selector_mouse, this, EM_TRUE, &native_on_mouse_event);
        emscripten_set_mouseup_callback(input_selector_mouse, this, EM_TRUE, &native_on_mouse_event);

        // Keyboard events
        emscripten_set_keydown_callback(input_selector_keyboard, this, EM_TRUE, &native_on_keypress);
        emscripten_set_keyup_callback(input_selector_keyboard, this, EM_TRUE, &native_on_keypress);
    }

    auto WebAsmApp::refresh_events() noexcept -> ice::Result
    {
        pthread_mutex_lock(&_mutex);
        ice::shards::clear(_system_events);

        _input_events._events = _temporary_inputs._events;
        ice::array::clear(_temporary_inputs._events);

        _text_events = _temp_text_events;
        ice::queue::clear(_temp_text_events);

        ice::queue::for_each(
            _text_events,
            [this](WebAsmTextEvent const& ev) noexcept
            {
                ICE_LOG(LogSeverity::Warning, LogTag::Core, "{}", ev.input);
                ice::shards::push_back(
                    _system_events,
                    ice::platform::ShardID_InputText | (char const*)ev.input
                );
            }
        );

        pthread_mutex_unlock(&_mutex);
        return S_Success;
    }

    void WebAsmApp::on_mouse_event(int type, EmscriptenMouseEvent const& event) noexcept
    {
        pthread_mutex_lock(&_mutex);
        switch(type)
        {
            using ice::input::DeviceMessage;
            using ice::input::MouseInput;
        case EMSCRIPTEN_EVENT_MOUSEMOVE:
            _temporary_inputs.push(Constant_Mouse, input::DeviceMessage::MousePositionX, event.targetX);
            _temporary_inputs.push(Constant_Mouse, input::DeviceMessage::MousePositionY, event.targetY);
            break;
        case EMSCRIPTEN_EVENT_MOUSEDOWN:
            if (event.button == 0) _temporary_inputs.push(Constant_Mouse, input::DeviceMessage::MouseButtonDown, MouseInput::ButtonLeft);
            else if (event.button == 1) _temporary_inputs.push(Constant_Mouse, input::DeviceMessage::MouseButtonDown, MouseInput::ButtonMiddle);
            else if (event.button == 2) _temporary_inputs.push(Constant_Mouse, input::DeviceMessage::MouseButtonDown, MouseInput::ButtonRight);
            break;
        case EMSCRIPTEN_EVENT_MOUSEUP:
            if (event.button == 0) _temporary_inputs.push(Constant_Mouse, input::DeviceMessage::MouseButtonUp, MouseInput::ButtonLeft);
            else if (event.button == 1) _temporary_inputs.push(Constant_Mouse, input::DeviceMessage::MouseButtonUp, MouseInput::ButtonMiddle);
            else if (event.button == 2) _temporary_inputs.push(Constant_Mouse, input::DeviceMessage::MouseButtonUp, MouseInput::ButtonRight);
            break;
        default: break;
        }
        pthread_mutex_unlock(&_mutex);
    }

    bool WebAsmApp::on_key_event(int type, EmscriptenKeyboardEvent const& event) noexcept
    {
        ICE_LOG(LogSeverity::Debug, LogTag::Core, "Press {} {}", event.code, event.key);

        bool send_text_event;
        KeyboardKey const key = webasm_map_keycode(event.code, event.key, send_text_event);

        if (key == KeyboardKey::Unknown)
        {
            return false;
        }

        // Skip all F keys for now
        if (key >= KeyboardKey::KeyF1 && key <= KeyboardKey::KeyF12)
        {
            return false;
        }

        KeyboardMod mod = KeyboardMod::None;
        switch(key)
        {
        case KeyboardKey::KeyLeftAlt: mod = KeyboardMod::AltLeft; break;
        case KeyboardKey::KeyRightAlt: mod = KeyboardMod::AltRight; break;
        case KeyboardKey::KeyLeftShift: mod = KeyboardMod::ShiftLeft; break;
        case KeyboardKey::KeyRightShift: mod = KeyboardMod::ShiftRight; break;
        case KeyboardKey::KeyLeftCtrl: mod = KeyboardMod::CtrlLeft; break;
        case KeyboardKey::KeyRightCtrl: mod = KeyboardMod::CtrlRight; break;
        default: break;
        }

        pthread_mutex_lock(&_mutex);
        if (type == EMSCRIPTEN_EVENT_KEYDOWN)
        {
            if (send_text_event)
            {
                WebAsmTextEvent text_event{.input = {}};
                ice::memcpy(text_event.input, event.key, sizeof(event.key));
                ice::queue::push_back(_temp_text_events, text_event);
            }
            if (mod != KeyboardMod::None)
            {
                _temporary_inputs.push(Constant_Keyboard, ice::input::DeviceMessage::KeyboardModifierDown, mod);
            }
            _temporary_inputs.push(Constant_Keyboard, ice::input::DeviceMessage::KeyboardButtonDown, key);
        }
        else if (type == EMSCRIPTEN_EVENT_KEYUP)
        {
            if (mod != KeyboardMod::None)
            {
                _temporary_inputs.push(Constant_Keyboard, ice::input::DeviceMessage::KeyboardModifierUp, mod);
            }
            _temporary_inputs.push(Constant_Keyboard, ice::input::DeviceMessage::KeyboardButtonUp, key);
        }
        pthread_mutex_unlock(&_mutex);

        return true;
    }

} // namespace ice::platform::webasm
