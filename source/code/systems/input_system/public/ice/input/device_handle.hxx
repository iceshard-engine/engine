/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>

namespace ice::input
{

    struct Device;

    enum class DeviceIndex : ice::u8;
    enum class DeviceType : ice::u8;
    enum class DeviceHandle : ice::u8;


    constexpr auto make_device_handle(
        ice::input::DeviceType type,
        ice::input::DeviceIndex index
    ) noexcept -> ice::input::DeviceHandle;

    constexpr auto make_device_handle(
        ice::input::Device device
    ) noexcept -> ice::input::DeviceHandle;

    constexpr auto make_device(
        ice::input::DeviceHandle handle
    ) noexcept -> ice::input::Device;


    enum class DeviceIndex : ice::u8 { };

    enum class DeviceType : ice::u8
    {
        Invalid = 0x0,
        Mouse,
        Keyboard,
        Controller,
        TouchScreen,

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

    constexpr auto make_device_handle(
        ice::input::Device device
    ) noexcept -> ice::input::DeviceHandle
    {
        if (std::is_constant_evaluated())
        {
            return ice::bit_cast<ice::input::DeviceHandle>(ice::u8(ice::u8(device.type) | ice::u8(device.index) << 4));
        }
        else
        {
            return ice::bit_cast<ice::input::DeviceHandle>(device);
        }
    }

    constexpr auto make_device(
        ice::input::DeviceHandle handle
    ) noexcept -> ice::input::Device
    {
        if (std::is_constant_evaluated())
        {
            ice::u8 const val = ice::bit_cast<ice::u8>(handle);
            Device result;
            result.type = ice::input::DeviceType(ice::u8(val & 0x0f));
            result.index = ice::input::DeviceIndex(ice::u8(val >> 4));
            return result;
        }
        else
        {
            return ice::bit_cast<ice::input::Device>(handle);
        }
    }

    constexpr auto make_device_handle(
        ice::input::DeviceType type,
        ice::input::DeviceIndex index
    ) noexcept -> ice::input::DeviceHandle
    {
        Device device;
        device.type = type;
        device.index = index;
        return make_device_handle(device);
    }

} // ice::input
