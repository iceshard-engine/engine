#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>
#include <iceshard/renderer/render_pass.hxx>
#include <iceshard/renderer/vulkan/vulkan_sdk.hxx>

namespace iceshard::renderer::vulkan
{

    enum class RenderPassAttachmentType
    {
        ColorTexture,
        ColorPresent,
        DepthStencil,
    };




    class VulkanRenderPass final
    {
    public:
        VulkanRenderPass() noexcept;
        ~VulkanRenderPass() noexcept;

    private:
        VkRenderPass _render_pass;
    };

    auto create_render_pass(core::allocator& alloc) noexcept -> core::memory::unique_pointer<VulkanRenderPass>;

} // namespace iceshard::renderer::vulkan
