#pragma once
#include <iceshard/renderer/vulkan/vulkan_surface.hxx>
#include <iceshard/renderer/vulkan/vulkan_devices.hxx>
#include <core/pod/array.hxx>

namespace iceshard::renderer::vulkan
{

    enum class VulkanSwapchain : uintptr_t
    {
        Invalid = 0x0
    };

    struct VulkanSwapchainImage
    {
        VkImage image;
        VkImageView view;
    };

    [[nodiscard]]
    auto swapchain_images(
        VulkanSwapchain swapchain
    ) noexcept -> core::pod::Array<VulkanSwapchainImage> const&;

    [[nodiscard]]
    auto native_handle(
        VulkanSwapchain swapchain
    ) noexcept -> VkSwapchainKHR;

    auto create_swapchain(
        core::allocator& alloc,
        VulkanDevices devices,
        VulkanSurface surface
    ) noexcept -> VulkanSwapchain;

    void destroy_swapchain(
        core::allocator& alloc,
        VulkanSwapchain swapchain
    ) noexcept;

} // namespace iceshard::renderer::vulkan
