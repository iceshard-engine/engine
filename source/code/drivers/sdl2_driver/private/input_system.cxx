#include <core/memory.hxx>
#include <core/pod/array.hxx>
#include <core/message/buffer.hxx>
#include <core/message/operations.hxx>

#include <input_system/system.hxx>
#include "event_handlers.hxx"

namespace input::sdl2
{

    class SDLInputSystem : public input::InputSystem
    {
        using EventTransformFunc = void(const SDL_Event&, core::MessageBuffer&) noexcept;

    public:
        SDLInputSystem(core::allocator& alloc) noexcept
            : _transforms{ alloc }
        {
            const bool sdl2_init_successfull = SDL_Init(0) == 0;
            if (sdl2_init_successfull == false)
            {
                IS_ASSERT(sdl2_init_successfull == true, "Initialization od SDL2 failed! Error: '{}'", SDL_GetError());
            }

            if (SDL_InitSubSystem(SDL_INIT_EVENTS) >= 0)
            {
                input::sdl2::register_app_event_handlers(_transforms);
                input::sdl2::register_window_event_handlers(_transforms);
                input::sdl2::register_mouse_event_handlers(_transforms);
                input::sdl2::register_keyboard_event_handlers(_transforms);
            }
        }

        ~SDLInputSystem() noexcept override
        {
            core::pod::hash::clear(_transforms);
            SDL_Quit();
        }

        //! \brief Queries the media driver for messages.
        void query_messages(core::MessageBuffer& message_buffer) const noexcept override
        {
            static SDL_Event current_event{ };
            while (SDL_PollEvent(&current_event) != 0)
            {
                if (auto transform_func = core::pod::hash::get<EventHandlerSignature*>(_transforms, current_event.type, nullptr))
                {
                    transform_func(message_buffer, current_event);
                }
            }
        }

    private:
        core::pod::Hash<EventHandlerSignature*> _transforms;
    };

} // input::sdl2


extern "C"
{
    __declspec(dllexport) auto create_input_system(core::allocator& alloc) -> input::InputSystem*
    {
        return alloc.make<input::sdl2::SDLInputSystem>(alloc);
    }

    __declspec(dllexport) void release_input_system(core::allocator& alloc, input::InputSystem* driver)
    {
        alloc.destroy(driver);
    }
}
