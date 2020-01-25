#include <iceshard/renderer/vulkan/vulkan_renderpass.hxx>
#include <iceshard/renderer/render_pass.hxx>
#include "definitions/vulkan_renderpass_definitions.hxx"

namespace iceshard::renderer::vulkan
{

    bool create_renderpass(
        VkDevice device,
        VkFormat format,
        RenderPassFeatures features,
        VulkanRenderPass& renderpass
    ) noexcept
    {
        if (RenderPassFeatures::None != features)
        {
            return false;
        }

        renderpass.device = device;
        renderpass.format = format;
        renderpass.renderpass = renderpass_forward(device, format);
        return true;
    }

    void destroy_renderpass(VulkanRenderPass renderpass) noexcept
    {
        vkDestroyRenderPass(renderpass.device, renderpass.renderpass, nullptr);
    }

} // namespace iceshard::renderer::vulkan