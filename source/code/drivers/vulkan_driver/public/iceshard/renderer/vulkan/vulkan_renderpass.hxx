#pragma once
#include <iceshard/renderer/render_types.hxx>
#include <iceshard/renderer/vulkan/vulkan_sdk.hxx>

namespace iceshard::renderer::vulkan
{

    struct VulkanRenderPass
    {
        VkDevice device;
        VkFormat format;
        RenderPassFeatures features;
        VkRenderPass renderpass;
    };

    bool create_renderpass(
        VkDevice device,
        VkFormat attachment_format,
        RenderPassFeatures features,
        VulkanRenderPass& renderpass
    ) noexcept;

    void destroy_renderpass(
        VulkanRenderPass render_pass
    ) noexcept;

} // namespace iceshard::renderer::vulkan
