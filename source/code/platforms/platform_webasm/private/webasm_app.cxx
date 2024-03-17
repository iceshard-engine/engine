/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "webasm_app.hxx"
#include <ice/input/input_mouse.hxx>

namespace ice::platform::webasm
{

    static constexpr ice::input::DeviceHandle Constant_Mouse = ice::input::make_device_handle(
        ice::input::DeviceType::Mouse, ice::input::DeviceIndex(0)
    );

    auto native_on_mouse_event(int type, EmscriptenMouseEvent const* event, void* userdata) -> EM_BOOL
    {
        WebAsmApp* app = reinterpret_cast<WebAsmApp*>(userdata);
        app->on_mouse_event(type, *event);
        return EM_TRUE;
    }

    WebAsmApp::WebAsmApp() noexcept
        : WebAsmCoreApp{ }
        , _temporary_inputs{ _allocator }
    {
        pthread_mutex_init(&_mutex, nullptr);
        emscripten_set_mousemove_callback("#canvas", this, EM_TRUE, &native_on_mouse_event);
        emscripten_set_mousedown_callback("#canvas", this, EM_TRUE, &native_on_mouse_event);
        emscripten_set_mouseup_callback("#canvas", this, EM_TRUE, &native_on_mouse_event);

        _temporary_inputs.push(Constant_Mouse, ice::input::DeviceMessage::DeviceConnected);
    }

    WebAsmApp::~WebAsmApp() noexcept
    {
        pthread_mutex_destroy(&_mutex);
    }

    auto WebAsmApp::refresh_events() noexcept -> ice::Result
    {
        pthread_mutex_lock(&_mutex);
        ice::array::clear(_input_events._events);
        _input_events._events = _temporary_inputs._events;
        ice::array::clear(_temporary_inputs._events);
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

} // namespace ice::platform::webasm
