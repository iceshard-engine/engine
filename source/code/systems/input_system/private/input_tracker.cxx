/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/input/input_tracker.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/assert_core.hxx>
#include "input_devices.hxx"

namespace ice::input
{

    class SimpleInputTracker final : public InputTracker
    {
    public:
        SimpleInputTracker(
            ice::Allocator& alloc,
            ice::Clock const& input_clock
        ) noexcept;
        ~SimpleInputTracker() noexcept override;

        void register_device_type(
            ice::input::DeviceType type,
            ice::input::DeviceFactory* device_factory
        ) noexcept override;

        void process_device_events(
            ice::Span<ice::input::DeviceEvent const> events,
            ice::Array<ice::input::InputEvent>& input_events_out
        ) noexcept override;

        void query_tracked_devices(
            ice::Array<ice::input::DeviceHandle>& devices_out
        ) const noexcept override;

    private:
        ice::Allocator& _allocator;
        ice::Timer _timer;

        ice::HashMap<DeviceFactory*> _factories;
        ice::HashMap<InputDevice*> _devices;
    };

    SimpleInputTracker::SimpleInputTracker(
        ice::Allocator& alloc,
        ice::Clock const& input_clock
    ) noexcept
        : InputTracker{ }
        , _allocator{ alloc }
        , _timer{ ice::timer::create_timer(input_clock, 10_Tms) }
        , _factories{ _allocator }
        , _devices{ _allocator }
    {
    }

    SimpleInputTracker::~SimpleInputTracker() noexcept
    {
        for (InputDevice* entry : _devices)
        {
            _allocator.destroy(entry);
        }
    }

    void SimpleInputTracker::register_device_type(
        ice::input::DeviceType type,
        ice::input::DeviceFactory* device_factory
    ) noexcept
    {
        // #todo handle duplicate type case
        if (ice::hashmap::has(_factories, ice::hash(type)) == false)
        {
            ice::hashmap::set(_factories, ice::hash(type), device_factory);
        }
    }

    void SimpleInputTracker::process_device_events(
        ice::Span<ice::input::DeviceEvent const> events,
        ice::Array<ice::input::InputEvent>& input_events_out
    ) noexcept
    {
        if (ice::timer::update(_timer))
        {
            for (InputDevice* device : _devices)
            {
                device->on_tick(_timer);
            }
        }

        for (ice::input::DeviceEvent event : events)
        {
            ice::u64 const device_hash = ice::hash(event.device);

            if (event.message == DeviceMessage::DeviceConnected)
            {
                ICE_ASSERT_CORE(ice::hashmap::get(_devices, device_hash, nullptr) == nullptr);

                Device const device = ice::input::make_device(event.device);
                DeviceFactory* const factory_func = ice::hashmap::get(
                    _factories,
                    ice::hash(device.type),
                    nullptr
                );

                if (factory_func != nullptr)
                {
                    InputDevice* const device_state = factory_func(_allocator, event.device);
                    ICE_ASSERT_CORE(device_state != nullptr);

                    ice::hashmap::set(_devices, device_hash, device_state);
                    if (device_state->max_count() > 1)
                    {
                        ice::hashmap::set(_devices, ice::hash(device.type), device_state);
                    }
                    // #todo log device connected (shard?)
                }
            }
            else if (event.message == DeviceMessage::DeviceDisconnected)
            {
                ICE_ASSERT_CORE(ice::hashmap::get(_devices, device_hash, nullptr) != nullptr);

                _allocator.destroy(
                    ice::hashmap::get(_devices, device_hash, nullptr)
                );

                ice::hashmap::remove(_devices, device_hash);
                // #todo log device disconnected (shard?)
            }
            else if (ice::hashmap::has(_devices, device_hash))
            {
                InputDevice* const device = ice::hashmap::get(_devices, device_hash, nullptr);
                device->on_event(event);
            }
            else
            {
                // Support multiple device handles in a single InputDevice
                Device const device_info = ice::input::make_device(event.device);
                InputDevice* const* const device_ptr = ice::hashmap::try_get(_devices, ice::hash(device_info.type));
                if (device_ptr != nullptr)
                {
                    InputDevice* const device = *device_ptr;
                    ICE_ASSERT_CORE(device->max_count() > 1);
                    device->on_event(event);
                }
            }
        }

        for (InputDevice* device : _devices)
        {
            device->on_publish(input_events_out);
        }
    }

    void SimpleInputTracker::query_tracked_devices(
        ice::Array<ice::input::DeviceHandle>& devices_out
    ) const noexcept
    {
        for (InputDevice* device : _devices)
        {
            ice::array::push_back(devices_out, device->handle());
        }
    }

    auto create_default_input_tracker(
        ice::Allocator& alloc,
        ice::Clock const& input_clock
    ) noexcept -> ice::UniquePtr<ice::input::InputTracker>
    {
        return ice::make_unique<ice::input::SimpleInputTracker>(alloc, alloc, input_clock);
    }

    auto default_device_factory_func(
        ice::Allocator& alloc,
        ice::input::DeviceHandle handle
    ) noexcept -> ice::input::InputDevice*
    {
        ice::input::Device const device = make_device(handle);
        switch (device.type)
        {
        case DeviceType::Mouse:
            return create_mouse_device(alloc, handle);
        case DeviceType::Keyboard:
            return create_keyboard_device(alloc, handle);
        case DeviceType::Controller:
            return create_controller_device(alloc, handle);
        case DeviceType::TouchScreen:
            return create_touchscreen_device(alloc, handle);
        default:
            return nullptr;
        }
    }

    auto get_default_device_factory() noexcept -> ice::input::DeviceFactory*
    {
        return ice::input::default_device_factory_func;
    }

} // namespace ice::input
