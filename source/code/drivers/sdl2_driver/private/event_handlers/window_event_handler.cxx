#include "../event_handlers.hxx"
#include <core/message/operations.hxx>
#include <input_system/message/window.hxx>

namespace iceshard::input::sdl2
{
    namespace detail
    {

        void window_event_handler(core::MessageBuffer& message_buffer, SDL_Event const& sdl_event) noexcept
        {
            auto const& window_event = sdl_event.window;
            switch (window_event.event)
            {
            case SDL_WindowEventID::SDL_WINDOWEVENT_SIZE_CHANGED:
                core::message::push(message_buffer, ::input::message::WindowSizeChanged
                    {
                        .width = static_cast<uint32_t const>(window_event.data1),
                        .height = static_cast<uint32_t const>(window_event.data2),
                    });
            default:
                break;
            }
        }

    } // namespace detail


    //! \brief Handler functions for application events.
    void register_window_event_handlers(core::pod::Hash<EventHandlerSignature*>& handler_map) noexcept
    {
        core::pod::hash::set(handler_map, SDL_EventType::SDL_WINDOWEVENT, &detail::window_event_handler);
    }

} // namespace input::sdl2
