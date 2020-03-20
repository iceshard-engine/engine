#pragma once
#include <iceshard/renderer/render_api.hxx>
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

    enum class VulkanFramebufferTexture : uint32_t
    {
        Attachment0,
        Attachment1,
        Attachment2,
        Attachment3,
    };

    auto framebuffer_texture_from_handle(iceshard::renderer::api::Texture handle) noexcept -> VulkanFramebufferTexture;

    auto framebuffer_image(VulkanFramebuffer framebuffer, VulkanFramebufferTexture texture) noexcept -> VkImageView;

    auto native_handle(VulkanFramebuffer framebuffer) noexcept -> VkFramebuffer;

    void create_framebuffers(
        core::allocator& alloc,
        VkExtent2D framebuffer_extent,
        VulkanDevices devices,
        VulkanRenderPass renderpass,
        VulkanSwapchain swapchain,
        core::pod::Array<VulkanFramebuffer>& framebuffers
    ) noexcept;

    void destroy_framebuffers(
        core::allocator& alloc,
        core::pod::Array<VulkanFramebuffer>& framebuffers
    ) noexcept;

} // namespace iceshard::renderer::vulkan
