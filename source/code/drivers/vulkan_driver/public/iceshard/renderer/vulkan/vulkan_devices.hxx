#pragma once
#include <iceshard/renderer/vulkan/vulkan_sdk.hxx>

namespace iceshard::renderer::vulkan
{

    struct VulkanDevices
    {
        VkPhysicalDevice physical_device;
        VkDevice graphics_device;
    };

    bool create_devices(VkInstance instance, VulkanDevices& devices) noexcept;

    void destroy_devices(VkInstance instance, VulkanDevices const& devices) noexcept;

} // namespace iceshard::renderer::vulkan
