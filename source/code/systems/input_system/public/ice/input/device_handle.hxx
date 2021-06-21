#pragma once
#include <ice/base.hxx>

namespace ice::input
{

    struct Device;

    enum class DeviceIndex : ice::u8;

    enum class DeviceType : ice::u8;

    enum class DeviceHandle : ice::u8;

    inline auto make_device_handle(
        ice::input::DeviceType type,
        ice::input::DeviceIndex index
    ) noexcept -> ice::input::DeviceHandle;

    inline auto make_device_handle(
        ice::input::Device device
    ) noexcept -> ice::input::DeviceHandle;

    inline auto make_device(
        ice::input::DeviceHandle handle
    ) noexcept -> ice::input::Device;



    enum class DeviceIndex : ice::u8 { };

    enum class DeviceType : ice::u8
    {
        Invalid = 0x0,
        Mouse,
        Keyboard,
        Controller,

        Reserved = 0x10
    };

    struct Device
    {
        DeviceType type : 4;
        DeviceIndex index : 4;
    };

    enum class DeviceHandle : ice::u8
    {
        Invalid = 0x0,
    };

    inline auto make_device_handle(
        ice::input::DeviceType type,
        ice::input::DeviceIndex index
    ) noexcept -> ice::input::DeviceHandle
    {
        Device device;
        device.type = type;
        device.index = index;
        return make_device_handle(device);
    }

    inline auto make_device_handle(
        ice::input::Device device
    ) noexcept -> ice::input::DeviceHandle
    {
        union
        {
            Device device;
            DeviceHandle device_handle;
        } converter{ .device = device };
        return converter.device_handle;
    }

    inline auto make_device(
        ice::input::DeviceHandle handle
    ) noexcept -> ice::input::Device
    {
        union
        {
            Device device;
            DeviceHandle device_handle;
        } converter{ .device_handle = handle };
        return converter.device;
    }

} // ice::input
