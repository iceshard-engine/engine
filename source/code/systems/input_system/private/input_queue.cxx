#include <iceshard/input/device/input_device_queue.hxx>

namespace iceshard::input
{
    DeviceInputQueue::DeviceInputQueue(core::allocator& alloc) noexcept
        : _input_messages{ alloc }
        , _input_data{ alloc }
    {
    }

    DeviceInputQueue::~DeviceInputQueue() noexcept
    {
    }

    void DeviceInputQueue::clear() noexcept
    {
        core::pod::array::clear(_input_messages);
        core::buffer::clear(_input_data);
    }

} // namespace iceshard::input
