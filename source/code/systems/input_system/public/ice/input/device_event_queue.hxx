/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

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

        template<typename T, DevicePayloadType PayloadType = ice::input::Constant_PayloadType<T>>
            requires (PayloadType != ice::input::DevicePayloadType::Invalid) && (std::is_enum_v<T> == false)
        inline void push(
            ice::input::DeviceHandle device,
            ice::input::DeviceMessage message,
            T payload_value
        ) noexcept;

        template<typename T> requires std::is_enum_v<T>
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
        return _events.is_empty();
    }

    inline void DeviceEventQueue::clear() noexcept
    {
        _events.clear();
    }

    inline void DeviceEventQueue::push(
        ice::input::DeviceHandle device,
        ice::input::DeviceMessage message
    ) noexcept
    {
        _events.push_back(
            DeviceEvent{
                .device = device,
                .message = message,
                .payload_type = ice::input::DevicePayloadType::Invalid,
            }
        );
    }

    template<typename T, DevicePayloadType PayloadType>
        requires (PayloadType != ice::input::DevicePayloadType::Invalid) && (std::is_enum_v<T> == false)
    inline void DeviceEventQueue::push(
        ice::input::DeviceHandle device,
        ice::input::DeviceMessage message,
        T payload_value
    ) noexcept
    {
        DeviceEvent event{
            .device = device,
            .message = message,
            .payload_type = PayloadType,
        };

        if constexpr (std::is_same_v<T, ice::i8>)
        {
            event.payload_data.val_i8 = payload_value;
        }
        else if constexpr (std::is_same_v<T, ice::i16>)
        {
            event.payload_data.val_i16 = payload_value;
        }
        else if constexpr (std::is_same_v<T, ice::math::vec<2, i16>>)
        {
            event.payload_data.val_i16x2 = payload_value;
        }
        else if constexpr (std::is_same_v<T, ice::i32>)
        {
            event.payload_data.val_i32 = payload_value;
        }
        else if constexpr (std::is_same_v<T, ice::f32>)
        {
            event.payload_data.val_f32 = payload_value;
        }

        _events.push_back(event);
    }

    template<typename T> requires std::is_enum_v<T>
    inline void DeviceEventQueue::push(
        ice::input::DeviceHandle device,
        ice::input::DeviceMessage message,
        T payload_value
    ) noexcept
    {
        this->push<std::underlying_type_t<T>, DevicePayloadType::Enum>(device, message, std::underlying_type_t<T>(payload_value));
    }

} // ice::input
