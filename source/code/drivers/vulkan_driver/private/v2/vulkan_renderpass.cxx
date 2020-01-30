#include <iceshard/renderer/vulkan/vulkan_renderpass.hxx>
#include <iceshard/renderer/render_pass.hxx>
#include "renderpass/vulkan_renderpass_definitions.hxx"

namespace iceshard::renderer::vulkan
{

    void get_renderpass_image_info(
        VulkanRenderPass renderpass,
        core::pod::Array<RenderPassImage>& image_infos
    ) noexcept
    {
        if (RenderPassFeatures::None != renderpass.features)
        {
            return;
        }

        core::pod::array::push_back(
            image_infos,
            RenderPassImage{
                .type = RenderPassImageType::SwapchainImage,
                .index = 0
            }
        );
        core::pod::array::push_back(
            image_infos,
            RenderPassImage{
                .type = RenderPassImageType::DepthStencilImage,
                .index = 1,
                .format = VK_FORMAT_D16_UNORM
            }
        );
    }

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
        renderpass.features = features;
        renderpass.renderpass = renderpass_forward(device, format);
        return true;
    }

    void destroy_renderpass(VulkanRenderPass renderpass) noexcept
    {
        vkDestroyRenderPass(renderpass.device, renderpass.renderpass, nullptr);
    }

} // namespace iceshard::renderer::vulkan