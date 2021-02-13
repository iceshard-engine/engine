#include "sdl2_app_container.hxx"
#include "sdl2_render_surface.hxx"
#include <ice/assert.hxx>
#include <ice/pod/array.hxx>
#include <ice/platform_event.hxx>
#include <ice/input/input_mouse.hxx>
#include <ice/input/device_queue.hxx>

#include <SDL.h>

namespace ice::platform
{

    namespace detail
    {

        using ice::input::MouseInput;

        auto map_sdl_mouse_button(ice::u8 button_id) noexcept
        {
            switch (button_id)
            {
            case SDL_BUTTON_LEFT:
                return MouseInput::ButtonLeft;
            case SDL_BUTTON_RIGHT:
                return MouseInput::ButtonRight;
            case SDL_BUTTON_MIDDLE:
                return MouseInput::ButtonMiddle;
            case SDL_BUTTON_X1:
                return MouseInput::ButtonCustom0;
            case SDL_BUTTON_X2:
                return MouseInput::ButtonCustom1;
            default:
                break;
            }
            return MouseInput::Unknown;
        }

        void mouse_input_events(
            ice::input::DeviceQueue& input_queue,
            SDL_Event const& sdl_event
        ) noexcept
        {
            using namespace ice::input;

            if (sdl_event.type == SDL_EventType::SDL_MOUSEMOTION)
            {
                ice::i32 pos[2];

                if (SDL_GetRelativeMouseMode())
                {
                    pos[0] = sdl_event.motion.xrel;
                    pos[1] = sdl_event.motion.yrel;
                }
                else
                {
                    pos[0] = sdl_event.motion.x;
                    pos[1] = sdl_event.motion.y;
                }

                if (pos[0] != 0 || pos[1] != 0)
                {
                    input_queue.push(
                        make_device_handle(DeviceType::Mouse, DeviceIndex(sdl_event.motion.which)),
                        DeviceMessage::MousePosition,
                        pos[0], pos[1]
                    );
                }
            }
            else if (sdl_event.type == SDL_MOUSEBUTTONDOWN)
            {
                input_queue.push(
                    make_device_handle(DeviceType::Mouse, DeviceIndex(sdl_event.motion.which)),
                    DeviceMessage::MouseButtonDown,
                    map_sdl_mouse_button(sdl_event.button.button)
                );
            }
            else if (sdl_event.type == SDL_MOUSEBUTTONUP)
            {
                input_queue.push(
                    make_device_handle(DeviceType::Mouse, DeviceIndex(sdl_event.motion.which)),
                    DeviceMessage::MouseButtonUp,
                    map_sdl_mouse_button(sdl_event.button.button)
                );
            }
            else if (sdl_event.type == SDL_MOUSEWHEEL)
            {
                input_queue.push(
                    make_device_handle(DeviceType::Mouse, DeviceIndex(sdl_event.motion.which)),
                    DeviceMessage::MouseWheel,
                    sdl_event.wheel.y
                );
            }
        }

    } // namespace detail

    SDL2_Container::SDL2_Container(
        ice::Allocator& alloc,
        ice::UniquePtr<ice::platform::App> app
    ) noexcept
        : Container{ }
        , _allocator{ alloc }
        , _app{ ice::move(app) }
    {
        [[maybe_unused]]
        bool const init_success = SDL_InitSubSystem(SDL_INIT_EVENTS) >= 0;
        ICE_ASSERT(init_success, "Initialization error for SDL2 'Events' subsystem.");
    }

    SDL2_Container::~SDL2_Container() noexcept
    {
        SDL_QuitSubSystem(SDL_INIT_EVENTS);
    }

    auto SDL2_Container::run() noexcept -> ice::i32
    {
        using namespace ice::input;

        ice::input::DeviceQueue device_events{ _allocator };
        ice::pod::Array<ice::platform::Event> events{ _allocator };

        device_events.push(
            make_device_handle(DeviceType::Mouse, DeviceIndex(0)),
            DeviceMessage::DeviceConnected
        );

        while (_request_quit == false)
        {
            static SDL_Event current_event{ };
            while (SDL_PollEvent(&current_event) != 0)
            {
                switch (current_event.type)
                {
                case SDL_QUIT:
                    ice::pod::array::push_back(events,
                        Event{
                            .type = EventType::AppQuit,
                            .data = { }
                        }
                    );

                    device_events.push(
                        make_device_handle(DeviceType::Mouse, DeviceIndex(0)),
                        DeviceMessage::DeviceDisconnected
                    );

                    _request_quit = true;
                    break;
                case SDL_MOUSEBUTTONUP:
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEWHEEL:
                case SDL_MOUSEMOTION:
                    detail::mouse_input_events(device_events, current_event);
                    break;
                }
            }

            _app->handle_inputs(device_events);
            _app->update(events);

            device_events.clear();
            ice::pod::array::clear(events);
        }

        return 0;
    }

    auto create_app_container(
        ice::Allocator& alloc,
        ice::UniquePtr<ice::platform::App> app
    ) noexcept -> ice::UniquePtr<ice::platform::Container>
    {
        ice::UniquePtr<Container> result = ice::make_unique_null<Container>();
        if (app != nullptr)
        {
            result = ice::make_unique<Container, SDL2_Container>(alloc, alloc, ice::move(app));
        }
        return result;
    }

} // namespace ice::platform