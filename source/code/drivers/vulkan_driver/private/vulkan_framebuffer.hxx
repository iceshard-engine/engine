#pragma once
#include <core/allocator.hxx>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <vulkan/vulkan.h>

#include "vulkan_swapchain.hxx"
#include "vulkan_image.hxx"

namespace render::vulkan
{

    class VulkanFramebuffer
    {
    public:
        VulkanFramebuffer(VkDevice device, VkFramebuffer framebuffer_handle) noexcept;
        ~VulkanFramebuffer() noexcept;

        auto native_handle() const noexcept -> VkFramebuffer { return _native_handle; }

    private:
        VkDevice _device_handle;
        VkFramebuffer _native_handle;
    };

    void create_framebuffers(
        core::allocator& alloc,
        core::pod::Array<VulkanFramebuffer*>& results,
        VkDevice device,
        VkRenderPass render_pass,
        VulkanImage const& depth_buffer,
        VulkanSwapchain const& swapchain,
        VulkanPhysicalDevice const* vulkan_physical_device) noexcept;

} // namespace render::vulkan
