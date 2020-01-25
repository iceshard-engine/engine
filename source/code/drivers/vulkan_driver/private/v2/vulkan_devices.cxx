#include <iceshard/renderer/vulkan/vulkan_devices.hxx>
#include "..\..\public\iceshard\renderer\vulkan\vulkan_devices.hxx"

namespace iceshard::renderer::vulkan
{

    bool create_devices(VkInstance instance, VulkanDevices& devices) noexcept
    {
        uint32_t physical_device_count;

        VkResult res = vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);
        IS_ASSERT(res == VK_SUCCESS, "Couldn't properly query the number of available vulkan devices!");

        fmt::print(stderr, "Warning : found more than one device! Picking first returned device by default!\n");
        physical_device_count = 1;

        vkEnumeratePhysicalDevices(instance, &physical_device_count, &devices.physical_device);
        IS_ASSERT(res == VK_SUCCESS, "Couldn't properly query available vulkan devices!");
        return true;
    }

    void destroy_devices(VkInstance, VulkanDevices const&) noexcept
    {
        // nothing to do yet
    }

} // namespace iceshard::renderer::vulkan
