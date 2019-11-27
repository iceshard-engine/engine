#include "../event_handlers.hxx"
#include <core/message/operations.hxx>
#include <input_system/message/mouse.hxx>

namespace input::sdl2
{
    namespace detail
    {

        auto map_sdl_button(uint8_t button_id) noexcept
        {
            switch (button_id)
            {
            case SDL_BUTTON_LEFT:
                return MouseButton::Left;
            case SDL_BUTTON_RIGHT:
                return MouseButton::Right;
            case SDL_BUTTON_MIDDLE:
                return MouseButton::Middle;
            default:
                break;
            }
            return MouseButton::Unknown;
        }

        void mouse_motion_event_handler(core::MessageBuffer& message_buffer, const SDL_Event& sdl_event) noexcept
        {
            core::message::push(message_buffer, input::message::MouseMotion{ input::message::MousePos{ sdl_event.motion.x, sdl_event.motion.y } });
        }

        void mouse_button_event_handler(core::MessageBuffer& message_buffer, const SDL_Event& sdl_event) noexcept
        {
            if (sdl_event.button.type == SDL_MOUSEBUTTONDOWN)
            {
                core::message::push(message_buffer,
                    input::message::MouseButtonDown{
                        .button = map_sdl_button(sdl_event.button.button),
                        .pos = input::message::MousePos{ sdl_event.motion.x, sdl_event.motion.y }
                    }
                );
            }
            else // if (sdl_event.button.state == SDL_MOUSEBUTTONUP)
            {
                core::message::push(message_buffer,
                    input::message::MouseButtonUp{
                        .button = map_sdl_button(sdl_event.button.button),
                        .pos = input::message::MousePos{ sdl_event.motion.x, sdl_event.motion.y }
                    }
                );
            }
        }

    } // namespace detail


    //! \brief Handler functions for mouse events.
    void register_mouse_event_handlers(core::pod::Hash<EventHandlerSignature*>& handler_map) noexcept
    {
        core::pod::hash::set(handler_map, SDL_EventType::SDL_MOUSEMOTION, &detail::mouse_motion_event_handler);
        core::pod::hash::set(handler_map, SDL_EventType::SDL_MOUSEBUTTONDOWN, &detail::mouse_button_event_handler);
        core::pod::hash::set(handler_map, SDL_EventType::SDL_MOUSEBUTTONUP, &detail::mouse_button_event_handler);
    }

} // namespace input::sdl2
