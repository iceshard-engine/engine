#include "../event_handlers.hxx"
#include <core/message/operations.hxx>
#include <input_system/message/app.hxx>

namespace input::sdl2
{
    namespace detail
    {

        //! \brief Handles the SDL_QUIT event.
        void quit_event_handler(core::MessageBuffer& message_buffer, SDL_Event const&) noexcept
        {
            core::message::push(message_buffer, input::message::AppExit{ });
        }

    } // namespace detail


    //! \brief Handler functions for application events.
    void register_app_event_handlers(core::pod::Hash<EventHandlerSignature*>& handler_map) noexcept
    {
        core::pod::hash::set(handler_map, SDL_EventType::SDL_QUIT, &detail::quit_event_handler);
    }

} // namespace input::sdl2
