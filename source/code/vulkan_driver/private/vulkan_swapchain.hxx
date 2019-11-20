#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>
#include <vulkan/vulkan.h>

#include "device/vulkan_physical_device.hxx"

namespace render::vulkan
{

    class VulkanSwapchain
    {
    public:
        VulkanSwapchain(VulkanDevice* vulkan_device, VkSwapchainKHR swapchain) noexcept;
        ~VulkanSwapchain() noexcept;

    private:
        VulkanDevice* _vulkan_device;
        VkSwapchainKHR _swapchain_handle;
    };

    auto create_swapchain(
        core::allocator& alloc,
        VulkanPhysicalDevice* vulkan_physical_device) noexcept -> core::memory::unique_pointer<VulkanSwapchain>;

} // namespace render::vulkan
