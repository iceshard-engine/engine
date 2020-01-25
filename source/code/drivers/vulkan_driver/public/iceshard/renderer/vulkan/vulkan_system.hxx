#pragma once
#include <core/allocator.hxx>
#include <iceshard/renderer/render_system.hxx>
#include <iceshard/renderer/vulkan/vulkan_sdk.hxx>
#include <iceshard/renderer/vulkan/vulkan_surface.hxx>
#include <iceshard/renderer/vulkan/vulkan_renderpass.hxx>

namespace iceshard::renderer::vulkan
{

    class VulkanRenderSystem final : iceshard::renderer::RenderSystem
    {
    public:
        VulkanRenderSystem(core::allocator& alloc, VkDevice device) noexcept;
        ~VulkanRenderSystem() noexcept override;

        void prepare(VkFormat format, RenderPassFeatures renderpass_features) noexcept;

        auto renderpass(RenderPassStage stage = RenderPassStage::Geometry) noexcept -> RenderPass;

        auto renderpass_native(RenderPassStage stage = RenderPassStage::Geometry) noexcept -> VkRenderPass
        {
            return iceshard::renderer::vulkan::native_handle(renderpass(stage));
        }

    private:
        core::allocator& _allocator;
        VkDevice const _vk_device;
        RenderPass _render_pass;

    };

    auto create_render_system(core::allocator& alloc, VkDevice device) noexcept -> VulkanRenderSystem*;

    void destroy_render_system(core::allocator& alloc, VulkanRenderSystem* system) noexcept;

} // namespace iceshard::renderer::vulkan
