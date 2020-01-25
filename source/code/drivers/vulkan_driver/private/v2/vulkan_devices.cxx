#include <iceshard/renderer/vulkan/vulkan_devices.hxx>
#include "..\..\public\iceshard\renderer\vulkan\vulkan_devices.hxx"

namespace iceshard::renderer::vulkan
{

    namespace detail
    {

        auto to_string(VkPhysicalDeviceType type) noexcept
        {
            switch (type)
            {
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                return "integrated-gpu";
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                return "discrete-gpu";
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                return "virtual-gpu";
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                return "cpu";
            default:
                return "unknown";
            }
        }

    } // namespace detail

    bool create_devices(VkInstance instance, VulkanDevices& devices) noexcept
    {
        uint32_t physical_device_count;

        VkResult res = vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);
        IS_ASSERT(res == VK_SUCCESS, "Couldn't properly query the number of available vulkan devices!");

        if (physical_device_count > 1)
        {
            fmt::print(stderr, "Warning : found more than one device! Picking first returned device by default!\n");
            physical_device_count = 1;
        }

        vkEnumeratePhysicalDevices(instance, &physical_device_count, &devices.physical_device);
        IS_ASSERT(res == VK_SUCCESS, "Couldn't properly query available vulkan devices!");

        VkPhysicalDeviceProperties physical_device_properties;
        vkGetPhysicalDeviceProperties(devices.physical_device, &physical_device_properties);

        fmt::print("Physical device properties:\n");
        fmt::print("- vendor.id: {}\n", physical_device_properties.vendorID);
        fmt::print("- device.id: {}\n", physical_device_properties.deviceID);
        fmt::print("- device.type: {}\n", detail::to_string(physical_device_properties.deviceType));
        fmt::print("- device.name: {}\n", physical_device_properties.deviceName);
        fmt::print("- version.driver: {}\n", physical_device_properties.driverVersion);
        fmt::print("- version.api: {}\n", physical_device_properties.apiVersion);
        return true;
    }

    void destroy_devices(VkInstance, VulkanDevices const&) noexcept
    {
        // nothing to do yet
    }

} // namespace iceshard::renderer::vulkan