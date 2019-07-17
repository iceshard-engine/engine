#include <core/memory.hxx>
#include <core/pod/array.hxx>
#include <core/pod/hash.hxx>
#include <core/message/buffer.hxx>
#include <core/message/operations.hxx>

#include <device/message/system.h>
#include <device/message/mouse.h>
#include <device/system.hxx>
#include <device/keyboard.hxx>
#include <device/driver.hxx>

#include <SDL.h>

void quit_event_transform(const SDL_Event& /*sdl_event*/, core::MessageBuffer& message_buffer) noexcept
{
    core::message::push(message_buffer, driver::message::AppExit{ });
}

void mouse_event_transform(const SDL_Event& sdl_event, core::MessageBuffer& message_buffer) noexcept
{
    core::message::push(message_buffer, driver::message::MouseMove{ sdl_event.motion.x, sdl_event.motion.y });
}

class SDLMediaDriver : public media::MediaDriver
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


namespace device
{

    class SDL2Keyboard : public KeyboardDevice
    {
    public:
        SDL2Keyboard() noexcept
        {
        }

        //! \brief Checks the given key state.
        bool check_pressed(KeyboardKey /*key*/) noexcept override
        {
            return false;
        }

        //! \brief Checks the given modifier state.
        bool check_modifier(KeyboardMod mod) noexcept override
        {
            switch (mod)
            {
            case device::KeyboardMod::ShiftAny:
                return (SDL_GetModState() & KMOD_SHIFT) != 0;
            case device::KeyboardMod::CtrlAny:
                return (SDL_GetModState() & KMOD_CTRL) != 0;
            case device::KeyboardMod::AltAny:
                return (SDL_GetModState() & KMOD_ALT) != 0;
            case device::KeyboardMod::GuiAny:
                return (SDL_GetModState() & KMOD_GUI) != 0;
            }
            return false;
        }
    };

} // namespace sdl2_driver


void query_devices(core::pod::Array<device::Device*>& device_list)
{
    core::pod::array::push_back<device::Device*>(device_list, nullptr);


}

auto get_api_internal() noexcept -> device::ProviderAPI*
{
    static device::ProviderAPI api_table {
        &query_devices
    };

    return &api_table;
}


extern "C"
{
    __declspec(dllexport) auto create_driver(core::allocator& alloc) -> media::MediaDriver*
    {
        return alloc.make<SDLMediaDriver>(alloc);
    }

    __declspec(dllexport) void release_driver(core::allocator& alloc, media::MediaDriver* driver)
    {
        alloc.destroy(driver);
    }
}
