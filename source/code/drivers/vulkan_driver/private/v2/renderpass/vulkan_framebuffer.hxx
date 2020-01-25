#pragma once
#include <iceshard/renderer/vulkan/vulkan_sdk.hxx>
#include <iceshard/renderer/vulkan/vulkan_renderpass.hxx>
#include <core/pod/array.hxx>

namespace iceshard::renderer::vulkan
{

    struct VulkanFramebuffersInfo;

    class VulkanFramebuffers final
    {
    public:
        VulkanFramebuffers(core::allocator& alloc, VulkanFramebuffersInfo info) noexcept;
        ~VulkanFramebuffers() noexcept;

    private:
        core::pod::Array<VkFramebuffer> _framebuffers;
    };

    auto create_framebuffers(core::allocator& alloc, VkRenderPass renderpass) noexcept;

} // namespace iceshard::renderer::vulkan
