#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <vulkan/vulkan.h>

#include "device/vulkan_physical_device.hxx"

namespace render::vulkan
{

    class VulkanSwapchain
    {
    public:
        VulkanSwapchain(core::allocator& alloc, VulkanPhysicalDevice* vulkan_device, VkSwapchainKHR swapchain) noexcept;
        ~VulkanSwapchain() noexcept;

        auto native_handle() const noexcept -> VkSwapchainKHR { return _swapchain_handle; }

    public:
        struct SwapchainBuffer
        {
            VkImage image;
            VkImageView view;
        };

        auto swapchain_buffers() const noexcept -> core::pod::Array<SwapchainBuffer> const& { return _swapchain_buffers; }

    protected:
        void initialize() noexcept;
        void shutdown() noexcept;

    private:
        core::allocator& _allocator;
        VulkanPhysicalDevice* _vulkan_physical_device;
        VkSwapchainKHR _swapchain_handle;

        core::pod::Array<SwapchainBuffer> _swapchain_buffers;
    };

    auto create_swapchain(
        core::allocator& alloc,
        VulkanPhysicalDevice* vulkan_physical_device) noexcept -> core::memory::unique_pointer<VulkanSwapchain>;

} // namespace render::vulkan
