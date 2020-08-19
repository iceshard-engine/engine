#pragma once
#include <iceshard/input/input_event.hxx>
#include <iceshard/input/device/input_device.hxx>
#include <iceshard/input/device/input_device_queue.hxx>
#include <core/clock.hxx>

namespace iceshard::input
{

    class DeviceState
    {
    public:
        virtual ~DeviceState() noexcept = default;

        virtual void on_tick() noexcept = 0;
        virtual void on_message(DeviceInputMessage msg, void const* data) noexcept = 0;
        virtual void on_publish(core::pod::Array<InputEvent>& events_out) noexcept = 0;
    };

    using DeviceStateFactory = auto (core::allocator&, DeviceHandle) noexcept -> DeviceState*;

    class DeviceStateManager final
    {
    public:
        DeviceStateManager(core::allocator& alloc, core::Clock const& clock) noexcept;
        ~DeviceStateManager() noexcept;

        void register_state_factory(
            DeviceType device_type,
            DeviceStateFactory* state_factory
        ) noexcept;

        void handle_device_inputs(
            DeviceInputQueue const& input_queue,
            core::pod::Array<InputEvent>& input_events_out
        ) noexcept;

        auto connected_devices() const noexcept -> core::pod::Array<DeviceHandle> const&;

    private:
        core::allocator& _allocator;
        core::Timer _timer;

        core::pod::Hash<DeviceStateFactory*> _state_factories;
        core::pod::Hash<DeviceState*> _states;
        core::pod::Array<DeviceHandle> _devices;
    };

} // namespace iceshard::input
