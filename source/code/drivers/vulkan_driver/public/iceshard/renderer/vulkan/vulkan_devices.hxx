#pragma once
#include <iceshard/renderer/vulkan/vulkan_sdk.hxx>

namespace iceshard::renderer::vulkan
{

    struct VulkanDevices
    {
        VkPhysicalDevice physical_device;
        VkDevice graphics_device;
    };

    bool find_memory_type_index(
        VulkanDevices devices,
        VkMemoryRequirements memory_requirements,
        VkMemoryPropertyFlags property_bits,
        uint32_t& type_index_out
    ) noexcept;

    bool create_devices(VkInstance instance, VulkanDevices& devices) noexcept;

    void destroy_devices(VkInstance instance, VulkanDevices const& devices) noexcept;

} // namespace iceshard::renderer::vulkan
