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

        void background_event_handler(core::MessageBuffer& message_buffer, SDL_Event const& sdl_event) noexcept
        {
            if (sdl_event.type == SDL_EventType::SDL_APP_WILLENTERBACKGROUND)
            {
                core::message::push(message_buffer, input::message::AppStatusChangeing{ input::message::AppStatus::InBackground });
            }
            else if (sdl_event.type == SDL_EventType::SDL_APP_DIDENTERBACKGROUND)
            {
                core::message::push(message_buffer, input::message::AppStatusChanged{ input::message::AppStatus::InBackground });
            }
        }

        void foreground_event_handler(core::MessageBuffer& message_buffer, SDL_Event const& sdl_event) noexcept
        {
            if (sdl_event.type == SDL_EventType::SDL_APP_WILLENTERFOREGROUND)
            {
                core::message::push(message_buffer, input::message::AppStatusChangeing{ input::message::AppStatus::InForeground });
            }
            else if (sdl_event.type == SDL_EventType::SDL_APP_DIDENTERFOREGROUND)
            {
                core::message::push(message_buffer, input::message::AppStatusChanged{ input::message::AppStatus::InForeground });
            }
        }

    } // namespace detail


    //! \brief Handler functions for application events.
    void register_app_event_handlers(core::pod::Hash<EventHandlerSignature*>& handler_map) noexcept
    {
        core::pod::hash::set(handler_map, SDL_EventType::SDL_QUIT, &detail::quit_event_handler);
        core::pod::hash::set(handler_map, SDL_EventType::SDL_APP_WILLENTERBACKGROUND, &detail::background_event_handler);
        core::pod::hash::set(handler_map, SDL_EventType::SDL_APP_DIDENTERBACKGROUND, &detail::background_event_handler);
        core::pod::hash::set(handler_map, SDL_EventType::SDL_APP_WILLENTERFOREGROUND, &detail::foreground_event_handler);
        core::pod::hash::set(handler_map, SDL_EventType::SDL_APP_DIDENTERFOREGROUND, &detail::foreground_event_handler);
    }

} // namespace input::sdl2