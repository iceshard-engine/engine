#pragma once
#include <iceshard/renderer/render_system.hxx>
#include <iceshard/renderer/render_pass.hxx>
#include <iceshard/renderer/vulkan/vulkan_sdk.hxx>

namespace iceshard::renderer::vulkan
{

    class VulkanRenderSystem : iceshard::renderer::RenderSystem
    {
    public:
        VulkanRenderSystem() noexcept = default;
        ~VulkanRenderSystem() noexcept override = default;

        auto viewport() noexcept -> RenderViewport& override { return *_viewport; }

        auto renderpass() noexcept -> RenderPassHandle override { return RenderPassHandle::Invalid; }

    private:
        RenderViewport* _viewport;
    };

} // namespace iceshard::renderer::vulkan
