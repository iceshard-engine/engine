#include <iceshard/input/input_state_manager.hxx>
#include <iceshard/input/input_event.hxx>
#include <iceshard/input/input_mouse.hxx>
#include <iceshard/input/input_keyboard.hxx>
#include <iceshard/input/input_controller.hxx>

#include <core/pod/algorithm.hxx>
#include <core/pod/hash.hxx>

namespace iceshard::input
{

    namespace detail
    {

        void unknown_on_tick(DeviceState&) noexcept { }

        void unknown_on_message(DeviceState&, DeviceInputMessage, void const*) noexcept { }

        void unknown_on_publish(DeviceState&, core::pod::Array<InputEvent>&) noexcept { }

        struct UnknownDeviceState : DeviceState
        {
            UnknownDeviceState() noexcept = default;

            void on_tick() noexcept override { }
            void on_message(DeviceInputMessage, void const*) noexcept override { }
            void on_publish(core::pod::Array<InputEvent>&) noexcept override { }
        };

        auto unknown_device_state_factory(core::allocator& alloc, DeviceHandle handle) noexcept -> DeviceState*
        {
            return alloc.make<UnknownDeviceState>();
        }

    } // namespace detail

    DeviceStateManager::DeviceStateManager(core::allocator& alloc, core::Clock<> const& clock) noexcept
        : _allocator{ alloc }
        , _timer{ core::timer::create_timer(clock, 0.01) }
        , _state_factories{ _allocator }
        , _states{ _allocator }
        , _devices{ _allocator }
    {
        register_state_factory(DeviceType::Keyboard, default_keyboard_state_factory);
        register_state_factory(DeviceType::Mouse, default_mouse_state_factory);
        register_state_factory(DeviceType::Controller, default_controller_state_factory);
    }

    DeviceStateManager::~DeviceStateManager() noexcept
    {
        for (auto const& entry : _states)
        {
            _allocator.destroy(entry.value);
        }
    }

    void DeviceStateManager::register_state_factory(DeviceType device_type, DeviceStateFactory* state_factory) noexcept
    {
        auto const device_type_hash = core::hash(device_type);
        IS_ASSERT(
            core::pod::hash::has(_state_factories, device_type_hash) == false,
            "Factory for device type already registered"
        );

        core::pod::hash::set(_state_factories, device_type_hash, state_factory);
    }

    void DeviceStateManager::handle_device_inputs(
        DeviceInputQueue const& input_queue,
        core::pod::Array<InputEvent>& input_events_out
    ) noexcept
    {
        if (core::timer::update(_timer))
        {
            for (auto const& state_entry : _states)
            {
                state_entry.value->on_tick();
            }
        }

        input_queue.for_each([this](DeviceInputMessage const msg, void const* data) noexcept
            {
                auto const device_hash = core::hash(msg.device);

                if (msg.input_type == DeviceInputType::DeviceConnected)
                {
                    auto const device = device_from_handle(msg.device);
                    auto const device_type_hash = core::hash(device.type);

                    IS_ASSERT(
                        core::pod::hash::has(_states, device_hash) == false,
                        "Device already tracked!"
                    );

                    auto const factory_func = core::pod::hash::get<DeviceStateFactory*>(
                        _state_factories,
                        device_type_hash,
                        detail::unknown_device_state_factory
                    );

                    auto* const device_state = factory_func(_allocator, msg.device);

                    core::pod::array::push_back(_devices, msg.device);
                    core::pod::hash::set(_states, device_hash, device_state);

                    fmt::print("Device connected: {}\n", device_hash);
                }
                else if (msg.input_type == DeviceInputType::DeviceDisconnected)
                {
                    IS_ASSERT(core::pod::hash::has(_states, device_hash) == true, "Device not tracked!");
                    _allocator.destroy(
                        core::pod::hash::get(_states, device_hash, nullptr)
                    );
                    core::pod::hash::remove(_states, device_hash);

                    core::pod::array::resize(
                        _devices, core::pod::remove_if(_devices, msg.device)
                    );

                    fmt::print("Device disconnected: {}\n", device_hash);
                }
                else if (core::pod::hash::has(_states, device_hash))
                {
                    auto* const state = core::pod::hash::get(_states, device_hash, nullptr);
                    state->on_message(msg, data);
                }
            });

        for (auto const& state_entry : _states)
        {
            state_entry.value->on_publish(input_events_out);
        }
    }

    auto DeviceStateManager::connected_devices() const noexcept -> core::pod::Array<DeviceHandle> const&
    {
        return _devices;
    }

} // namespace iceshard::input
