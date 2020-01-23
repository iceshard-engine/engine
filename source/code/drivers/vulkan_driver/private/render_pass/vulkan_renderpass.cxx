#include "vulkan_renderpass.hxx"
#include <core/debug/assert.hxx>
#include <iceshard/renderer/vulkan/vulkan_renderpass.hxx>

namespace render::vulkan
{

    VulkanRenderPass::VulkanRenderPass(VkDevice device, VkRenderPass render_pass) noexcept
        : _device_handle{ device }
        , _native_handle{ render_pass }
    {
    }

    VulkanRenderPass::~VulkanRenderPass() noexcept
    {
        vkDestroyRenderPass(_device_handle, _native_handle, nullptr);
    }

    auto create_render_pass(core::allocator& alloc, VkDevice device, VkFormat color_format, VkFormat) noexcept -> core::memory::unique_pointer<VulkanRenderPass>
    {
        return core::memory::make_unique<VulkanRenderPass>(alloc, device,
            iceshard::renderer::vulkan::create_renderpass<iceshard::renderer::RenderPassType::Forward>(device, color_format)
        );
    }

} // namespace render::vulkan

