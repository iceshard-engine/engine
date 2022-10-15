#pragma once
#include <ice/input/device_event.hxx>
#include <ice/container/array.hxx>

namespace ice::input
{

    struct DeviceEventQueue
    {
        inline DeviceEventQueue(ice::Allocator& alloc) noexcept;
        inline ~DeviceEventQueue() noexcept = default;

        inline bool empty() const noexcept;
        inline void clear() noexcept;

        inline void push(
            ice::input::DeviceHandle device,
            ice::input::DeviceMessage message
        ) noexcept;

        template<typename T>
            requires (ice::input::Constant_PayloadType<T> != ice::input::DevicePayloadType::Invalid)
        inline void push(
            ice::input::DeviceHandle device,
            ice::input::DeviceMessage message,
            T payload_value
        ) noexcept;

        ice::Array<ice::input::DeviceEvent> _events;
    };

    inline DeviceEventQueue::DeviceEventQueue(ice::Allocator& alloc) noexcept
        : _events{ alloc }
    {
    }

    inline bool DeviceEventQueue::empty() const noexcept
    {
        return ice::array::empty(_events);
    }

    inline void DeviceEventQueue::clear() noexcept
    {
        ice::array::clear(_events);
    }

    inline void DeviceEventQueue::push(
        ice::input::DeviceHandle device,
        ice::input::DeviceMessage message
    ) noexcept
    {
        ice::array::push_back(
            _events,
            DeviceEvent{
                .device = device,
                .message = message,
                .payload_type = ice::input::DevicePayloadType::Invalid,
            }
        );
    }

    template<typename T>
        requires (ice::input::Constant_PayloadType<T> != ice::input::DevicePayloadType::Invalid)
    inline void DeviceEventQueue::push(
        ice::input::DeviceHandle device,
        ice::input::DeviceMessage message,
        T payload_value
    ) noexcept
    {
        ice::array::push_back(
            _events,
            DeviceEvent{
                .device = device,
                .message = message,
                .payload_type = ice::input::Constant_PayloadType<T>,
                .payload_data = { payload_value }
            }
        );
    }

} // ice::input
