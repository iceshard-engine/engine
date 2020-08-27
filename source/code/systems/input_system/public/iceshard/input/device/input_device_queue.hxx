#pragma once
#include <core/base.hxx>
#include <core/pod/array.hxx>
#include <core/data/buffer.hxx>
#include <core/memory.hxx>

#include <iceshard/input/device/input_device.hxx>
#include <iceshard/input/device/input_device_data.hxx>
#include <iceshard/input/device/input_device_message.hxx>

namespace iceshard::input
{

    namespace detail
    {

        template<typename T>
        auto read_internal(DeviceInputMessage message, void const* data) noexcept -> core::pod::Array<T const>
        {
            static constexpr auto data_info = input_data_info<T>;
            static_assert(data_info.type != 0x0, "Input data type is not declared");
            static_assert(data_info.bytes != 0x0, "Input data type has invalid size == 0");
            static_assert(data_info.count == 0x0, "Input data type has invalid default count != 0");

            IS_ASSERT(message.input_data.type == data_info.type, "Message data type does not match requested data type!");

            T const* data_ptr = reinterpret_cast<T const*>(data);
            return core::pod::array::create_view(data_ptr, message.input_data.count);
        }

    } // namespace detail

    class DeviceInputQueue
    {
    public:
        DeviceInputQueue(core::allocator& alloc) noexcept;
        ~DeviceInputQueue() noexcept;

        void clear() noexcept;

        bool empty() const noexcept;

        template<typename... Args>
        void push(DeviceHandle device, DeviceInputType input_type, Args... values) noexcept
        {
            using T = typename detail::first_type_from_list<Args...>::type;

            if constexpr (std::is_enum_v<T>)
            {
                push(device, input_type, static_cast<std::underlying_type_t<T>>(values)...);
            }
            else
            {
                static constexpr auto data_info = detail::input_data_info<T>;
                static_assert(data_info.type != 0x0, "Input data type is not declared");
                static_assert(data_info.bytes != 0x0 || sizeof...(values) == 0, "Input data type has invalid size == 0");
                static_assert(data_info.count == 0x0, "Input data type has invalid default count != 0");

                DeviceInputMessage msg{
                    .device = device,
                    .input_type = input_type,
                    .input_data = data_info,
                };
                msg.input_data.count = sizeof...(values);

                core::pod::array::push_back(_input_messages, msg);

                if constexpr (sizeof...(values) > 0)
                {
                    T const values_array[] = {
                        values...
                    };

                    core::buffer::append_aligned(_input_data, values_array, sizeof(values_array), 4);
                }
            }
        }

        template<typename Fn>
        void for_each(Fn&& fn) const noexcept
        {
            auto const* it = core::buffer::begin(_input_data);

            for (DeviceInputMessage const msg : _input_messages)
            {
                // Call the handler function
                fn(msg, it);

                // Move the data pointer
                auto const data_byte_count = msg.input_data.bytes * msg.input_data.count;

                it = core::memory::utils::align_forward(reinterpret_cast<char const*>(it) + data_byte_count, 4);
            }
        }

    private:
        core::pod::Array<DeviceInputMessage> _input_messages;
        core::Buffer _input_data;
    };

    template<typename T>
    auto read(DeviceInputMessage message, void const* data) noexcept
    {
        return detail::read_internal<T>(message, data);
    }

    template<typename T>
    auto read_one(DeviceInputMessage msg, void const* data) noexcept
    {
        if constexpr (std::is_enum_v<T>)
        {
            return T{ read<std::underlying_type_t<T>>(msg, data)[0] };
        }
        else
        {
            return read<T>(msg, data)[0];
        }
    }

} // namespace iceshard::input
