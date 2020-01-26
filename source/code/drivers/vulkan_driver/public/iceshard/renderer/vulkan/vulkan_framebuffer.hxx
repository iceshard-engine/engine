#pragma once
#include <iceshard/renderer/vulkan/vulkan_devices.hxx>
#include <iceshard/renderer/vulkan/vulkan_renderpass.hxx>
#include <iceshard/renderer/vulkan/vulkan_swapchain.hxx>
#include <core/pod/array.hxx>

namespace iceshard::renderer::vulkan
{

    enum class VulkanFramebuffer : uintptr_t
    {
        Invalid = 0x0
    };

    auto create_framebuffers(
        core::allocator& alloc,
        VulkanDevices devices,
        VulkanSurface surface,
        VulkanRenderPass renderpass,
        VulkanSwapchain swapchain,
        core::pod::Array<VulkanFramebuffer>& framebuffers
    ) noexcept;

    void destroy_framebuffers(
        core::allocator& alloc,
        core::pod::Array<VulkanFramebuffer> const& framebuffers
    ) noexcept;

} // namespace iceshard::renderer::vulkan
