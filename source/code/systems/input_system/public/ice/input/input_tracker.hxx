#pragma once
#include <ice/mem_unique_ptr.hxx>
#include <ice/input/device_queue.hxx>
#include <ice/input/input_event.hxx>
#include <ice/input/input_device.hxx>

namespace ice::input
{

    static constexpr ice::i32 default_button_event_treshold_hold = 40;
    static constexpr ice::i32 default_button_event_treshold_repeat = 20;

    using DeviceFactory = auto (
        ice::Allocator&,
        ice::input::DeviceHandle
    ) noexcept -> ice::input::InputDevice*;

    class InputTracker
    {
    public:
        virtual ~InputTracker() noexcept = default;

        virtual void register_device_type(
            ice::input::DeviceType type,
            ice::input::DeviceFactory* device_factory
        ) noexcept = 0;

        virtual void process_device_queue(
            ice::input::DeviceQueue const& event_queue,
            ice::Array<ice::input::InputEvent>& input_events_out
        ) noexcept = 0;

        virtual void query_tracked_devices(
            ice::Array<ice::input::DeviceHandle>& devices_out
        ) const noexcept = 0;
    };

    auto create_default_input_tracker(
        ice::Allocator& alloc,
        ice::Clock const& input_clock
    ) noexcept -> ice::UniquePtr<ice::input::InputTracker>;

    auto get_default_device_factory() noexcept -> ice::input::DeviceFactory*;

} // ice::input
