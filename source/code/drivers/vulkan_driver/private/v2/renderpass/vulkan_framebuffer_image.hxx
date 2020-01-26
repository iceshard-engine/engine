#pragma once
#include <iceshard/renderer/vulkan/vulkan_devices.hxx>

namespace iceshard::renderer::vulkan
{

    struct VulkanFramebufferImage
    {
        VkImageView image_view = vk_nullptr;
        VkImage allocated_image = vk_nullptr;
        VkDeviceMemory allocated_image_memory = vk_nullptr;
    };

    auto create_framebuffer_image(
        VulkanDevices device,
        VkExtent2D extent,
        VkFormat format,
        VkImageUsageFlags usage
    ) noexcept -> VulkanFramebufferImage;

} // namespace iceshard::renderer::vulkan
