#include <ice/input/device_queue.hxx>

namespace ice::input
{

    DeviceQueue::DeviceQueue(ice::Allocator& alloc) noexcept
        : _queue{ alloc }
        , _data{ alloc }
    {
    }

    bool DeviceQueue::empty() const noexcept
    {
        return ice::pod::array::empty(_queue);
    }

    void DeviceQueue::clear() noexcept
    {
        ice::pod::array::clear(_queue);
        ice::buffer::clear(_data);
    }

    void DeviceQueue::push(
        ice::input::DeviceEvent event,
        ice::Data payload
    ) noexcept
    {
        ice::pod::array::push_back(_queue, event);
        if (event.payload.type != 0)
        {
            ice::buffer::append(_data, payload);
        }
    }

} // namespace ice::input
