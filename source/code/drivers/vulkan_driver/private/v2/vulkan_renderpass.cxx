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
        if (renderpass.features == RenderPassFeatures::PostProcess)
        {
            core::pod::array::push_back(
                image_infos,
                RenderPassImage{
                    .type = RenderPassImageType::ColorImage,
                    .index = 0,
                    .format = VK_FORMAT_B8G8R8A8_UNORM,
                }
            );
            core::pod::array::push_back(
                image_infos,
                RenderPassImage{
                    .type = RenderPassImageType::SwapchainImage,
                    .index = 1
                }
            );
            core::pod::array::push_back(
                image_infos,
                RenderPassImage{
                    .type = RenderPassImageType::DepthStencilImage,
                    .index = 2,
                    .format = VK_FORMAT_D24_UNORM_S8_UINT
                }
            );
        }
        else
        {
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
                    .format = VK_FORMAT_D24_UNORM_S8_UINT
                }
            );
        }
    }

    bool create_renderpass(
        VkDevice device,
        VkFormat format,
        RenderPassFeatures features,
        VulkanRenderPass& renderpass
    ) noexcept
    {
        if (RenderPassFeatures::PostProcess == features)
        {
            renderpass.device = device;
            renderpass.format = format;
            renderpass.features = features;
            renderpass.renderpass = renderpass_forward_postprocess(device, format);
        }
        else
        {
            renderpass.device = device;
            renderpass.format = format;
            renderpass.features = features;
            renderpass.renderpass = renderpass_forward(device, format);
        }
        return true;
    }

    void destroy_renderpass(VulkanRenderPass renderpass) noexcept
    {
        vkDestroyRenderPass(renderpass.device, renderpass.renderpass, nullptr);
    }

} // namespace iceshard::renderer::vulkan