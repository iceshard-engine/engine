#include <iceshard/input/device/input_device.hxx>

namespace iceshard::input
{

    auto create_device_handle(Device device) noexcept -> DeviceHandle
    {
        union
        {
            Device device;
            DeviceHandle device_handle;
        } helper{ device };
        return helper.device_handle;
    }

    auto create_device_handle(uint8_t index, DeviceType type) noexcept -> DeviceHandle
    {
        IS_ASSERT(index <= 15, "Device index too high!");
        Device device;
        device.index = DeviceIndex{ index };
        device.type = type;
        return create_device_handle(device);
    }

    auto device_from_handle(DeviceHandle handle) noexcept -> Device
    {
        union
        {
            DeviceHandle device_handle;
            Device device;
        } helper{ handle };
        return helper.device;
    }

    bool is_device_type(DeviceHandle handle, DeviceType type) noexcept
    {
        union
        {
            DeviceHandle device_handle;
            Device device;
        } helper{ handle };
        return helper.device.type == type;
    }

} // namespace iceshard::input
