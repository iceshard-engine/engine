#include "../event_handlers.hxx"
#include <core/message/operations.hxx>
#include <input_system/message/mouse.hxx>

namespace input::sdl2
{
    namespace detail
    {

        void mouse_motion_event_handler(core::MessageBuffer& message_buffer, const SDL_Event& sdl_event) noexcept
        {
            core::message::push(message_buffer, input::message::MouseMotion{ sdl_event.motion.x, sdl_event.motion.y });
        }

    } // namespace detail


    //! \brief Handler functions for mouse events.
    void register_mouse_event_handlers(core::pod::Hash<EventHandlerSignature*>& handler_map) noexcept
    {
        core::pod::hash::set(handler_map, SDL_EventType::SDL_MOUSEMOTION, &detail::mouse_motion_event_handler);
    }

} // namespace input::sdl2
