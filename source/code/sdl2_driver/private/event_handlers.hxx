#pragma once
#include <core/pod/hash.hxx>
#include <core/message/buffer.hxx>
#include <SDL.h>

namespace input::sdl2
{


    //! \brief Event handler signature.
    using EventHandlerSignature = void(core::MessageBuffer&, const SDL_Event&) noexcept;


    //! \brief Handler functions for application events.
    void register_app_event_handlers(core::pod::Hash<EventHandlerSignature*>& handler_map) noexcept;

    //! \brief Handler functions for mouse events.
    void register_mouse_event_handlers(core::pod::Hash<EventHandlerSignature*>& handler_map) noexcept;

    //! \brief Handler functions for keyboard events.
    void register_keyboard_event_handlers(core::pod::Hash<EventHandlerSignature*>& handler_map) noexcept;


} // input::sdl2
