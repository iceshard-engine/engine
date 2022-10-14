#pragma once
#include <ice/mem_data.hxx>
#include <ice/clock.hxx>
#include <ice/container/array.hxx>
#include <ice/input/device_event.hxx>
#include <ice/input/input_event.hxx>

namespace ice::input
{

    class InputDevice
    {
    public:
        virtual ~InputDevice() noexcept = default;

        virtual auto handle() const noexcept -> ice::input::DeviceHandle = 0;

        virtual void on_tick(
            ice::Timer const& timer
        ) noexcept = 0;

        virtual void on_event(
            ice::input::DeviceEvent event,
            ice::Data payload
        ) noexcept = 0;

        virtual void on_publish(
            ice::Array<ice::input::InputEvent>& events_out
        ) noexcept = 0;
    };

} // ice::input
