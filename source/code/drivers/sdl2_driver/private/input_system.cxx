#include <core/memory.hxx>
#include <core/pod/array.hxx>
#include <core/pod/queue.hxx>
#include <core/message/buffer.hxx>
#include <core/message/operations.hxx>

#include <input_system/system.hxx>
#include "event_handlers.hxx"

namespace iceshard::input::sdl2
{

    namespace detail
    {

        struct SDLController
        {
            uint8_t device_id;
            SDL_GameController* sdl_data;
        };

    } // namespace detail

    class SDLInputSystem : public ::input::InputSystem
    {
        using EventTransformFunc = void(const SDL_Event&, core::MessageBuffer&) noexcept;

    public:
        SDLInputSystem(core::allocator& alloc) noexcept
            : _free_indices{ alloc }
            , _tracked_devices{ alloc }
            , _transforms{ alloc }
            , _transforms_new{ alloc }
        {
            core::pod::queue::reserve(_free_indices, 16);
            for (uint8_t i = 0; i < 16; ++i)
            {
                core::pod::queue::push_back(_free_indices, i);
            }

            const bool sdl2_init_successfull = SDL_Init(0) == 0;
            if (sdl2_init_successfull == false)
            {
                IS_ASSERT(sdl2_init_successfull == true, "Initialization od SDL2 failed! Error: '{}'", SDL_GetError());
            }

            SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
            if (SDL_InitSubSystem(SDL_INIT_EVENTS) >= 0)
            {
                iceshard::input::sdl2::register_app_event_handlers(_transforms);
                iceshard::input::sdl2::register_window_event_handlers(_transforms);
                iceshard::input::sdl2::register_mouse_event_handlers(_transforms_new);
                iceshard::input::sdl2::register_keyboard_event_handlers(_transforms_new, _transforms);
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

        void query_events(
            core::MessageBuffer& message_buffer,
            iceshard::input::DeviceInputQueue& input_queue
        ) noexcept override
        {
            static SDL_Event current_event{ };
            while (SDL_PollEvent(&current_event) != 0)
            {
                if (auto transform_func = core::pod::hash::get<InputHandlerSignature*>(_transforms_new, current_event.type, nullptr))
                {
                    transform_func(input_queue, current_event);
                }
                else if (auto transform_func = core::pod::hash::get<EventHandlerSignature*>(_transforms, current_event.type, nullptr))
                {
                    transform_func(message_buffer, current_event);
                }

                if (current_event.type == SDL_CONTROLLERDEVICEADDED)
                {
                    if (core::pod::queue::size(_free_indices) > 0)
                    {
                        SDL_GameController* const controller_data = SDL_GameControllerOpen(current_event.cdevice.which);
                        if (controller_data != nullptr)
                        {
                            uint8_t device_index = _free_indices[0];
                            core::pod::queue::pop_front(_free_indices);

                            uint64_t const controller_id = static_cast<uint64_t>(
                                SDL_JoystickInstanceID(
                                    SDL_GameControllerGetJoystick(controller_data)
                                )
                            );
                            IS_ASSERT(
                                core::pod::hash::has(_tracked_devices, controller_id) == false,
                                "Controller already added"
                            );

                            core::pod::hash::set(_tracked_devices, controller_id, detail::SDLController{
                                device_index, controller_data
                            });

                            input_queue.push(
                                iceshard::input::create_device_handle(device_index, iceshard::input::DeviceType::Controller),
                                iceshard::input::DeviceInputType::DeviceConnected
                            );
                        }
                    }
                }
                else if (current_event.type == SDL_CONTROLLERDEVICEREMOVED)
                {
                    uint64_t const controller_id = static_cast<uint64_t>(
                        current_event.cdevice.which
                    );
                    IS_ASSERT(
                        core::pod::hash::has(_tracked_devices, controller_id) == true,
                        "Removing unknown controller"
                    );

                    detail::SDLController controller = core::pod::hash::get(
                        _tracked_devices,
                        controller_id,
                        detail::SDLController{ 0, nullptr }
                    );

                    if (controller.sdl_data != nullptr)
                    {
                        core::pod::hash::remove(_tracked_devices, controller_id);
                        SDL_GameControllerClose(controller.sdl_data);

                        core::pod::queue::push_front(_free_indices, controller.device_id);
                        input_queue.push(
                            iceshard::input::create_device_handle(controller.device_id, iceshard::input::DeviceType::Controller),
                            iceshard::input::DeviceInputType::DeviceDisconnected
                        );
                    }
                }
            }
        }

    private:
        core::pod::Queue<uint8_t> _free_indices;
        core::pod::Hash<detail::SDLController> _tracked_devices;

        core::pod::Hash<EventHandlerSignature*> _transforms;
        core::pod::Hash<InputHandlerSignature*> _transforms_new;
    };

} // input::sdl2


extern "C"
{
    __declspec(dllexport) auto create_input_system(core::allocator& alloc) -> ::input::InputSystem*
    {
        return alloc.make<iceshard::input::sdl2::SDLInputSystem>(alloc);
    }

    __declspec(dllexport) void release_input_system(core::allocator& alloc, ::input::InputSystem* driver)
    {
        alloc.destroy(driver);
    }
}
