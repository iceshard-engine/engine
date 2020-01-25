#include "vulkan_renderpass_definitions.hxx"

namespace iceshard::renderer::vulkan
{

    auto renderpass_forward_postprocess(VkDevice device, VkFormat attachment_format) noexcept -> VkRenderPass
    {
        VkAttachmentDescription attachments[3]{ };
        attachments[0].flags = 0;
        attachments[0].format = attachment_format;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        attachments[1].flags = 0;
        attachments[1].format = attachment_format;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        attachments[2].flags = 0;
        attachments[2].format = VK_FORMAT_D16_UNORM;
        attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[2].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference references[4];
        references[0].attachment = 0;
        references[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        references[1].attachment = 0;
        references[1].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        references[2].attachment = 1;
        references[2].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        references[3].attachment = 2;
        references[3].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpasses[2];
        subpasses[0].flags = 0;
        subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpasses[0].inputAttachmentCount = 0;
        subpasses[0].pInputAttachments = nullptr;
        subpasses[0].colorAttachmentCount = 1;
        subpasses[0].pColorAttachments = references;
        subpasses[0].pResolveAttachments = nullptr;
        subpasses[0].pDepthStencilAttachment = references + 3;
        subpasses[0].preserveAttachmentCount = 0;
        subpasses[0].pPreserveAttachments = nullptr;

        subpasses[1].flags = 0;
        subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpasses[1].inputAttachmentCount = 1;
        subpasses[1].pInputAttachments = references + 1;
        subpasses[1].colorAttachmentCount = 1;
        subpasses[1].pColorAttachments = references + 2;
        subpasses[1].pResolveAttachments = nullptr;
        subpasses[1].pDepthStencilAttachment = nullptr;
        subpasses[1].preserveAttachmentCount = 0;
        subpasses[1].pPreserveAttachments = nullptr;

        VkSubpassDependency dependencies[1];
        dependencies[0].srcSubpass = 0;
        dependencies[0].dstSubpass = 1;
        dependencies[0].srcStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].dstStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[0].srcAccessMask = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        dependencies[0].dependencyFlags = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo rp_info = {};
        rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rp_info.pNext = nullptr;
        rp_info.attachmentCount = 3;
        rp_info.pAttachments = attachments;
        rp_info.subpassCount = 2;
        rp_info.pSubpasses = subpasses;
        rp_info.dependencyCount = 1;
        rp_info.pDependencies = dependencies;

        VkRenderPass render_pass;
        auto api_result = vkCreateRenderPass(device, &rp_info, nullptr, &render_pass);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create render pass!");

        return render_pass;
    }


} // namespace iceshard::renderer::vulkan
