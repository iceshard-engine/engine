#include "../event_handlers.hxx"
#include <core/message/operations.hxx>
#include <iceshard/input/device/input_device_queue.hxx>
#include <iceshard/input/input_mouse.hxx>
#include <iceshard/input/input_controller.hxx>

namespace iceshard::input::sdl2
{
    namespace detail
    {

        using iceshard::input::MouseInput;

        auto map_sdl_mouse_button(uint8_t button_id) noexcept
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

        auto map_sdl_gamepad_button(uint8_t button_id) noexcept
        {
            switch (button_id)
            {
            case SDL_CONTROLLER_BUTTON_A:
                return ControllerInput::ButtonA;
            case SDL_CONTROLLER_BUTTON_B:
                return ControllerInput::ButtonB;
            case SDL_CONTROLLER_BUTTON_X:
                return ControllerInput::ButtonX;
            case SDL_CONTROLLER_BUTTON_Y:
                return ControllerInput::ButtonY;
            default:
                break;
            }
            return ControllerInput::Unknown;
        }

        void mouse_input_events(iceshard::input::DeviceInputQueue& input_queue, SDL_Event const& sdl_event) noexcept
        {
            using namespace iceshard::input;

            if (sdl_event.type == SDL_EventType::SDL_MOUSEMOTION)
            {
                int32_t pos[2];

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
                        iceshard::input::create_device_handle(sdl_event.motion.which, DeviceType::Mouse),
                        DeviceInputType::MousePosition,
                        pos[0], pos[1]
                    );
                }
            }
            else if (sdl_event.type == SDL_MOUSEBUTTONDOWN)
            {
                input_queue.push(
                    iceshard::input::create_device_handle(sdl_event.button.which, DeviceType::Mouse),
                    DeviceInputType::MouseButtonDown,
                    map_sdl_mouse_button(sdl_event.button.button)
                );
            }
            else if (sdl_event.type == SDL_MOUSEBUTTONUP)
            {
                input_queue.push(
                    iceshard::input::create_device_handle(sdl_event.button.which, DeviceType::Mouse),
                    DeviceInputType::MouseButtonUp,
                    map_sdl_mouse_button(sdl_event.button.button)
                );
            }
            else if (sdl_event.type == SDL_MOUSEWHEEL)
            {
                input_queue.push(
                    iceshard::input::create_device_handle(sdl_event.wheel.which, DeviceType::Mouse),
                    DeviceInputType::MouseWheel,
                    sdl_event.wheel.y
                );
            }
        }

        void controller_input_events(iceshard::input::DeviceInputQueue& input_queue, SDL_Event const& sdl_event) noexcept
        {
            if (sdl_event.type == SDL_CONTROLLERBUTTONDOWN)
            {
                input_queue.push(
                    iceshard::input::create_device_handle(sdl_event.cbutton.which, DeviceType::Controller),
                    DeviceInputType::GamepadButtonDown,
                    map_sdl_gamepad_button(sdl_event.cbutton.button)
                );
            }
            else if (sdl_event.type == SDL_CONTROLLERBUTTONUP)
            {
                input_queue.push(
                    iceshard::input::create_device_handle(sdl_event.cbutton.which, DeviceType::Controller),
                    DeviceInputType::GamepadButtonUp,
                    map_sdl_gamepad_button(sdl_event.cbutton.button)
                );
            }
            else if (sdl_event.type == SDL_CONTROLLERAXISMOTION)
            {
                switch (sdl_event.caxis.axis)
                {
                case SDL_CONTROLLER_AXIS_LEFTX:
                    input_queue.push(
                        iceshard::input::create_device_handle(sdl_event.caxis.which, DeviceType::Controller),
                        DeviceInputType::GamepadAxisLeftX,
                        static_cast<float>(sdl_event.caxis.value) / 32767.0f
                    );
                    break;
                case SDL_CONTROLLER_AXIS_LEFTY:
                    input_queue.push(
                        iceshard::input::create_device_handle(sdl_event.caxis.which, DeviceType::Controller),
                        DeviceInputType::GamepadAxisLeftY,
                        static_cast<float>(sdl_event.caxis.value) / 32767.0f
                    );
                    break;
                case SDL_CONTROLLER_AXIS_RIGHTX:
                    input_queue.push(
                        iceshard::input::create_device_handle(sdl_event.caxis.which, DeviceType::Controller),
                        DeviceInputType::GamepadAxisRightX,
                        static_cast<float>(sdl_event.caxis.value) / 32767.0f
                    );
                    break;
                case SDL_CONTROLLER_AXIS_RIGHTY:
                    input_queue.push(
                        iceshard::input::create_device_handle(sdl_event.caxis.which, DeviceType::Controller),
                        DeviceInputType::GamepadAxisRightY,
                        static_cast<float>(sdl_event.caxis.value) / 32767.0f
                    );
                    break;
                case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
                    input_queue.push(
                        iceshard::input::create_device_handle(sdl_event.caxis.which, DeviceType::Controller),
                        DeviceInputType::GamepadTriggerLeft,
                        static_cast<float>(sdl_event.caxis.value) / 32767.0f
                    );
                    break;
                case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
                    input_queue.push(
                        iceshard::input::create_device_handle(sdl_event.caxis.which, DeviceType::Controller),
                        DeviceInputType::GamepadTriggerRight,
                        static_cast<float>(sdl_event.caxis.value) / 32767.0f
                    );
                    break;
                default:
                    break;
                }
            }
        }

    } // namespace detail

    //! \brief Handler functions for mouse events.
    void register_mouse_event_handlers(core::pod::Hash<InputHandlerSignature*>& handler_map) noexcept
    {
        core::pod::hash::set(handler_map, SDL_EventType::SDL_MOUSEMOTION, &detail::mouse_input_events);
        core::pod::hash::set(handler_map, SDL_EventType::SDL_MOUSEBUTTONDOWN, &detail::mouse_input_events);
        core::pod::hash::set(handler_map, SDL_EventType::SDL_MOUSEBUTTONUP, &detail::mouse_input_events);
        core::pod::hash::set(handler_map, SDL_EventType::SDL_MOUSEWHEEL, &detail::mouse_input_events);

        core::pod::hash::set(handler_map, SDL_EventType::SDL_CONTROLLERBUTTONDOWN, &detail::controller_input_events);
        core::pod::hash::set(handler_map, SDL_EventType::SDL_CONTROLLERBUTTONUP, &detail::controller_input_events);
        core::pod::hash::set(handler_map, SDL_EventType::SDL_CONTROLLERAXISMOTION, &detail::controller_input_events);
    }

} // namespace input::sdl2
