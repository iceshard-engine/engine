#pragma once
#include <iceshard/renderer/render_types.hxx>
#include <iceshard/renderer/vulkan/vulkan_sdk.hxx>
#include <core/pod/array.hxx>

namespace iceshard::renderer::vulkan
{

    enum class RenderPassImageType : uint32_t
    {
        ColorImage,
        SwapchainImage,
        DepthStencilImage,
    };

    struct RenderPassImage
    {
        RenderPassImageType type;
        uint32_t index;
        VkFormat format = VkFormat::VK_FORMAT_UNDEFINED;
    };

    struct VulkanRenderPass
    {
        VkDevice device;
        VkFormat format;
        RenderPassFeatures features;

        union
        {
            VkRenderPass renderpass;
            RenderPass handle;
        };
    };

    void get_renderpass_image_info(
        VulkanRenderPass renderpass,
        core::pod::Array<RenderPassImage>& image_infos
    ) noexcept;

    bool create_renderpass(
        VkDevice device,
        VkFormat attachment_format,
        RenderPassFeatures features,
        VulkanRenderPass& renderpass
    ) noexcept;

    void destroy_renderpass(
        VulkanRenderPass renderpass
    ) noexcept;

} // namespace iceshard::renderer::vulkan
