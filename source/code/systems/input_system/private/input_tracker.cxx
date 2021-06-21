#include <ice/input/input_tracker.hxx>
#include <ice/pod/hash.hxx>
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

        void process_device_queue(
            ice::input::DeviceQueue const& event_queue,
            ice::pod::Array<ice::input::InputEvent>& input_events_out
        ) noexcept override;

        void query_tracked_devices(
            ice::pod::Array<ice::input::DeviceHandle>& devices_out
        ) const noexcept override;

    private:
        ice::Allocator& _allocator;
        ice::Timer _timer;

        ice::pod::Hash<DeviceFactory*> _factories;
        ice::pod::Hash<InputDevice*> _devices;
    };

    SimpleInputTracker::SimpleInputTracker(
        ice::Allocator& alloc,
        ice::Clock const& input_clock
    ) noexcept
        : InputTracker{ }
        , _allocator{ alloc }
        , _timer{ ice::timer::create_timer(input_clock, 0.01) }
        , _factories{ _allocator }
        , _devices{ _allocator }
    {
    }

    SimpleInputTracker::~SimpleInputTracker() noexcept
    {
        for (auto const& entry : _devices)
        {
            _allocator.destroy(entry.value);
        }
    }

    void SimpleInputTracker::register_device_type(
        ice::input::DeviceType type,
        ice::input::DeviceFactory* device_factory
    ) noexcept
    {
        // #todo handle duplicate type case
        if (ice::pod::hash::has(_factories, ice::hash(type)) == false)
        {
            ice::pod::hash::set(_factories, ice::hash(type), device_factory);
        }
    }

    void SimpleInputTracker::process_device_queue(
        ice::input::DeviceQueue const& event_queue,
        ice::pod::Array<ice::input::InputEvent>& input_events_out
    ) noexcept
    {
        if (ice::timer::update(_timer))
        {
            for (auto const& device : _devices)
            {
                device.value->on_tick(_timer);
            }
        }

        for (auto[event, data] : event_queue)
        {
            ice::u64 const device_hash = ice::hash(event.device);

            if (event.message == DeviceMessage::DeviceConnected)
            {
                // #todo assert device already tracked

                Device const device = ice::input::make_device(event.device);
                DeviceFactory* const factory_func = ice::pod::hash::get<DeviceFactory*>(
                    _factories,
                    ice::hash(device.type),
                    nullptr
                );

                if (factory_func != nullptr)
                {
                    InputDevice* const device_state = factory_func(_allocator, event.device);
                    // #todo assert not null

                    ice::pod::hash::set(_devices, device_hash, device_state);
                    // #todo log device connected
                }
            }
            else if (event.message == DeviceMessage::DeviceDisconnected)
            {
                // #todo assert device NOT tracked
                _allocator.destroy(
                    ice::pod::hash::get(_devices, device_hash, nullptr)
                );

                ice::pod::hash::remove(_devices, device_hash);
                // #todo log device disconnected
            }
            else if (ice::pod::hash::has(_devices, device_hash))
            {
                InputDevice* const device = ice::pod::hash::get(_devices, device_hash, nullptr);
                device->on_event(event, data);
            }
        }

        for (auto const& device : _devices)
        {
            device.value->on_publish(input_events_out);
        }
    }

    void SimpleInputTracker::query_tracked_devices(
        ice::pod::Array<ice::input::DeviceHandle>& devices_out
    ) const noexcept
    {
        for (auto const& device : _devices)
        {
            ice::pod::array::push_back(devices_out, device.value->handle());
        }
    }

    auto create_default_input_tracker(
        ice::Allocator& alloc,
        ice::Clock const& input_clock
    ) noexcept -> ice::UniquePtr<ice::input::InputTracker>
    {
        return ice::make_unique<ice::input::InputTracker, ice::input::SimpleInputTracker>(alloc, alloc, input_clock);
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
        default:
            return nullptr;
        }
    }

    auto get_default_device_factory() noexcept -> ice::input::DeviceFactory*
    {
        return ice::input::default_device_factory_func;
    }

} // namespace ice::input
