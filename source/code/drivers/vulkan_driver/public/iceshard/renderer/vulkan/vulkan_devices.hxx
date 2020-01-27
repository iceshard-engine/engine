#pragma once
#include <iceshard/renderer/vulkan/vulkan_sdk.hxx>

namespace iceshard::renderer::vulkan
{

    struct VulkanPhysicalDevice
    {
        VkPhysicalDevice handle;
    };

    struct VulkanGraphicsDevice
    {
        VkDevice handle;
        VkQueue queue;
    };

    struct VulkanDevices
    {
        VulkanPhysicalDevice physical;
        VulkanGraphicsDevice graphics;
        VkQueue presenting_queue;
    };

    bool find_memory_type_index(
        VulkanDevices devices,
        VkMemoryRequirements memory_requirements,
        VkMemoryPropertyFlags property_bits,
        uint32_t& type_index_out
    ) noexcept;

    bool create_devices(VkInstance instance, VkSurfaceKHR surface, VulkanDevices& devices) noexcept;

    void destroy_devices(VkInstance instance, VulkanDevices const& devices) noexcept;

} // namespace iceshard::renderer::vulkan
