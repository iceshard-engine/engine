#include <iceshard/renderer/vulkan/vulkan_renderpass.hxx>
#include <iceshard/renderer/vulkan/vulkan_sdk.hxx>

namespace iceshard::renderer::vulkan
{

    template<>
    auto create_renderpass<RenderPassType::Forward>(VkDevice device, VkFormat attachment_format) noexcept -> VkRenderPass
    {
        VkAttachmentDescription attachments[2]{ };
        attachments[0].flags = 0;
        attachments[0].format = attachment_format;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        attachments[1].flags = 0;
        attachments[1].format = VK_FORMAT_D16_UNORM;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference references[2];
        references[0].attachment = 0;
        references[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        references[1].attachment = 1;
        references[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpasses[1];
        subpasses[0].flags = 0;
        subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpasses[0].inputAttachmentCount = 0;
        subpasses[0].pInputAttachments = nullptr;
        subpasses[0].colorAttachmentCount = 1;
        subpasses[0].pColorAttachments = references;
        subpasses[0].pResolveAttachments = nullptr;
        subpasses[0].pDepthStencilAttachment = references + 1;
        subpasses[0].preserveAttachmentCount = 0;
        subpasses[0].pPreserveAttachments = nullptr;

        VkRenderPassCreateInfo renderpass_info = {};
        renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderpass_info.pNext = nullptr;
        renderpass_info.attachmentCount = 2;
        renderpass_info.pAttachments = attachments;
        renderpass_info.subpassCount = 1;
        renderpass_info.pSubpasses = subpasses;
        renderpass_info.dependencyCount = 0;
        renderpass_info.pDependencies = nullptr;

        VkRenderPass render_pass;
        auto api_result = vkCreateRenderPass(device, &renderpass_info, nullptr, &render_pass);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create render pass!");

        return render_pass;
    }


} // namespace iceshard::renderer::vulkan
