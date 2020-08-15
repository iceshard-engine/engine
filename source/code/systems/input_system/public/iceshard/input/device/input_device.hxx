#pragma once
#include <core/base.hxx>

namespace iceshard::input
{

    enum class DeviceIndex : uint8_t { };

    enum class DeviceType : uint8_t
    {
        Invalid = 0x0,
        Mouse,
        Keyboard,
        Controller,
    };

    struct Device
    {
        DeviceIndex index : 4;
        DeviceType type : 4;
    };

    enum class DeviceHandle : uint8_t
    {
        Invalid = 0x0,
    };

    static_assert(sizeof(Device) == sizeof(DeviceHandle));

    auto create_device_handle(Device device) noexcept -> DeviceHandle;

    auto create_device_handle(uint8_t index, DeviceType type) noexcept -> DeviceHandle;

    auto device_from_handle(DeviceHandle handle) noexcept -> Device;

    bool is_device_type(DeviceHandle handle, DeviceType type) noexcept;

} // namespace iceshard::input

template<>
constexpr inline auto core::hash<iceshard::input::DeviceHandle>(iceshard::input::DeviceHandle handle) noexcept -> uint64_t
{
    return static_cast<uint64_t>(handle);
}
