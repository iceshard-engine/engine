#pragma once
#include <ice/buffer.hxx>
#include <ice/pod/array.hxx>
#include <ice/input/device_event.hxx>

namespace ice::input
{

    class DeviceQueue final
    {
    public:
        DeviceQueue(ice::Allocator& alloc) noexcept;
        ~DeviceQueue() noexcept = default;

        bool empty() const noexcept;

        void clear() noexcept;

        void push(
            ice::input::DeviceEvent event,
            ice::Data payload
        ) noexcept;

        template<typename... Args>
        inline void push(
            ice::input::DeviceHandle device,
            ice::input::DeviceMessage message,
            Args... args
        ) noexcept;


        struct Iterator
        {
            ice::pod::Array<DeviceEvent> const* _queue;
            ice::Data _data;
            ice::u32 _index = 0;
            ice::u32 _data_offset = 0;

            bool operator==(Iterator const& other) const noexcept
            {
                return _queue == other._queue && _index == other._index;
            }

            bool operator!=(Iterator const& other) const noexcept
            {
                return (*this == other) == false;
            }

            void operator++() noexcept
            {
                _index += 1;
            }

            auto operator*() noexcept -> std::tuple<DeviceEvent, Data>
            {
                DeviceEvent current_event = (*_queue)[_index];

                Data current_data;
                current_data.alignment = 4;
                current_data.location = ice::memory::ptr_add(_data.location, _data_offset);
                current_data.size = current_event.payload.size * current_event.payload.count;
                _data_offset += current_data.size;
                if ((_data_offset % 4) != 0)
                {
                    _data_offset += (4 - (_data_offset % 4));
                }

                return { current_event, current_data };
            }
        };

        inline auto begin() const noexcept -> Iterator;
        inline auto end() const noexcept -> Iterator;

    private:
        ice::pod::Array<DeviceEvent> _queue;
        ice::Buffer _data;
    };


    namespace detail
    {

        template<typename T = void, typename...>
        struct FirstType
        {
            using Type = T;
        };

    } // namespace detail

    template<typename ...Args>
    inline void DeviceQueue::push(
        ice::input::DeviceHandle device,
        ice::input::DeviceMessage message,
        Args... values
    ) noexcept
    {
        using PayloadType = typename detail::FirstType<Args...>::Type;

        if constexpr (std::is_same_v<PayloadType, void>)
        {
            this->push(
                DeviceEvent{
                    .device = device,
                    .message = message,
                    .payload = DeviceEventPayload{ .type = 0x0 }
                },
                Data{ .location = nullptr, .size = 0 }
            );
        }
        else
        {
            Data payload_data{ };

            if constexpr (std::is_enum_v<PayloadType> == true)
            {
                this->push(
                    device,
                    message,
                    static_cast<std::underlying_type_t<PayloadType>>(values)...
                );
            }
            else
            {
                constexpr DeviceEventPayload payload_info = detail::payload_info<PayloadType>;
                static_assert(payload_info.type != 0x0, "Payload type is not supported");
                static_assert(payload_info.size == sizeof(PayloadType) || sizeof...(values) == 0, "Payload type has invalid size == 0");
                static_assert(payload_info.count == 0x0, "Payload type has invalid default count != 0");

                PayloadType const values_array[] = {
                    values...
                };

                DeviceEventPayload payload = payload_info;
                payload.count = sizeof...(values);
                payload_data.location = &values_array;
                payload_data.size = payload.size * payload.count;
                payload_data.alignment = 4;

                // #todo assert payload_data.size == sizeof(values_array)

                this->push(
                    DeviceEvent{
                        .device = device,
                        .message = message,
                        .payload = payload
                    },
                    payload_data
                );
            }
        }
    }

    inline auto DeviceQueue::begin() const noexcept -> Iterator
    {
        return Iterator{
            ._queue = &_queue,
            ._data = _data,
            ._index = 0,
            ._data_offset = 0,
        };
    }

    inline auto DeviceQueue::end() const noexcept -> Iterator
    {
        return Iterator{
            ._queue = &_queue,
            ._data = { },
            ._index = ice::pod::array::size(_queue),
            ._data_offset = 0,
        };
    }

} // ice::input
