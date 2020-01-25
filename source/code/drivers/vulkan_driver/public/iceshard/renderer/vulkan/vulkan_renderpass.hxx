#pragma once
#include <core/allocator.hxx>
#include <iceshard/renderer/render_pass.hxx>
#include <iceshard/renderer/vulkan/vulkan_sdk.hxx>

namespace iceshard::renderer::vulkan
{

    auto native_handle(RenderPass renderpass) noexcept -> VkRenderPass;

    auto create_renderpass(
        VkDevice device,
        VkFormat attachment_format,
        RenderPassFeatures features
    ) noexcept -> RenderPass;

    void destroy_renderpass(
        VkDevice device,
        RenderPass render_pass
    ) noexcept;

} // namespace iceshard::renderer::vulkan
