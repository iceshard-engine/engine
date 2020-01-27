#pragma once
#include <iceshard/renderer/vulkan/vulkan_sdk.hxx>
#include <iceshard/renderer/vulkan/vulkan_devices.hxx>
#include <core/pod/array.hxx>

namespace iceshard::renderer::vulkan
{

    struct VulkanDeviceQueueFamily
    {
        uint32_t queue_index;
        uint32_t queue_count;
        bool supports_presenting = false;
        bool supports_graphics = false;
        bool supports_compute = false;
        bool supports_transfer = false;
    };

    void query_physical_device_queue_families(
        VkPhysicalDevice physical_device,
        VkSurfaceKHR surface,
        core::pod::Array<VulkanDeviceQueueFamily>& queues
    ) noexcept;

    void create_graphics_device(
        core::pod::Array<VulkanDeviceQueueFamily> const& queue_families,
        VkPhysicalDevice physical_device,
        VulkanGraphicsDevice& device
    ) noexcept;

    void release_graphics_device(
        VulkanGraphicsDevice device
    ) noexcept;

} // namespace iceshard::renderer::vulkan
