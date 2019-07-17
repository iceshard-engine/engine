#include <core/memory.hxx>
#include <core/pod/array.hxx>
#include <core/pod/hash.hxx>
#include <core/message/buffer.hxx>
#include <core/message/operations.hxx>

#include <input_system/message/mouse.hxx>
#include <input_system/message/app.hxx>
#include <input_system/module.hxx>

#include <SDL.h>

void quit_event_transform(const SDL_Event& /*sdl_event*/, core::MessageBuffer& message_buffer) noexcept
{
    core::message::push(message_buffer, input::message::AppExit{ });
}

void mouse_event_transform(const SDL_Event& sdl_event, core::MessageBuffer& message_buffer) noexcept
{
    core::message::push(message_buffer, input::message::MouseMove{ sdl_event.motion.x, sdl_event.motion.y });
}

class SDLMediaDriver : public input::InputQuery
{
    using EventTransformFunc = void(const SDL_Event&, core::MessageBuffer&) noexcept;

public:
    SDLMediaDriver(core::allocator& alloc) noexcept
        : _transforms{ alloc }
    {
        if (SDL_Init(SDL_INIT_VIDEO) >= 0)
        {
            core::pod::hash::set(_transforms, SDL_EventType::SDL_QUIT, &quit_event_transform);
            core::pod::hash::set(_transforms, SDL_EventType::SDL_MOUSEMOTION, &mouse_event_transform);
        }

        SDL_CreateWindow(
            "IceShard - Test window",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            1025, 756,
            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
        );
    }

    ~SDLMediaDriver() noexcept override
    {
        core::pod::hash::clear(_transforms);
        SDL_Quit();
    }

    //! \brief Updates the internal media driver state.
    void update() noexcept override
    {
    }

    //! \brief Queries the media driver for messages.
    void query_messages(core::MessageBuffer& message_buffer) const noexcept override
    {
        static SDL_Event current_event{ };
        while (SDL_PollEvent(&current_event) != 0)
        {
            if (auto transform_func = core::pod::hash::get<EventTransformFunc*>(_transforms, current_event.type, nullptr))
            {
                transform_func(current_event, message_buffer);
            }
        }
    }

private:
    core::pod::Hash<EventTransformFunc*> _transforms;
};


extern "C"
{
    __declspec(dllexport) auto create_driver(core::allocator& alloc) -> input::InputQuery*
    {
        return alloc.make<SDLMediaDriver>(alloc);
    }

    __declspec(dllexport) void release_driver(core::allocator& alloc, input::InputQuery* driver)
    {
        alloc.destroy(driver);
    }
}
