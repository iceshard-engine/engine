#pragma once
#include <core/pod/hash.hxx>
#include <core/message/buffer.hxx>
#include <SDL.h>

#include <iceshard/input/device/input_device_queue.hxx>

namespace iceshard::input::sdl2
{


    //! \brief Event handler signature.
    using EventHandlerSignature = void(core::MessageBuffer&, const SDL_Event&) noexcept;
    using InputHandlerSignature = void(iceshard::input::DeviceInputQueue&, const SDL_Event&) noexcept;


    //! \brief Handler functions for application events.
    void register_app_event_handlers(core::pod::Hash<EventHandlerSignature*>& handler_map) noexcept;

    //! \brief Handler functions for window events.
    void register_window_event_handlers(core::pod::Hash<EventHandlerSignature*>& handler_map) noexcept;

    //! \brief Handler functions for mouse events.
    void register_mouse_event_handlers(core::pod::Hash<InputHandlerSignature*>& handler_map) noexcept;

    //! \brief Handler functions for keyboard events.
    void register_keyboard_event_handlers(
        core::pod::Hash<InputHandlerSignature*>& inputs_map,
        core::pod::Hash<EventHandlerSignature*>& handler_map
    ) noexcept;


} // input::sdl2
