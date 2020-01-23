#include "vulkan_renderpass.hxx"
#include <core/debug/assert.hxx>
#include "..\v2\renderpass\vulkan_renderpass.hxx"

namespace render::vulkan
{

    VulkanRenderPass::VulkanRenderPass(VkDevice device, VkRenderPass render_pass) noexcept
        : _device_handle{ device }
        , _native_handle{ render_pass }
    {
    }

    VulkanRenderPass::~VulkanRenderPass() noexcept
    {
        vkDestroyRenderPass(_device_handle, _native_handle, nullptr);
    }

    auto create_render_pass(core::allocator& alloc, VkDevice device, VkFormat color_format, VkFormat depth_format) noexcept -> core::memory::unique_pointer<VulkanRenderPass>
    {
        VkAttachmentDescription attachments[2];
        attachments[0].format = color_format;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        attachments[0].flags = 0;

        attachments[1].format = depth_format;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments[1].flags = 0;

        VkAttachmentReference color_reference = {};
        color_reference.attachment = 0;
        color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depth_reference = {};
        depth_reference.attachment = 1;
        depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.flags = 0;
        subpass.inputAttachmentCount = 0;
        subpass.pInputAttachments = nullptr;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_reference;
        subpass.pResolveAttachments = nullptr;
        subpass.pDepthStencilAttachment = &depth_reference;
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments = nullptr;

        VkRenderPassCreateInfo rp_info = {};
        rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rp_info.pNext = nullptr;
        rp_info.attachmentCount = 2;
        rp_info.pAttachments = attachments;
        rp_info.subpassCount = 1;
        rp_info.pSubpasses = &subpass;
        rp_info.dependencyCount = 0;
        rp_info.pDependencies = nullptr;

        VkRenderPass render_pass;
        auto api_result = vkCreateRenderPass(device, &rp_info, nullptr, &render_pass);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create render pass!");

        return core::memory::make_unique<VulkanRenderPass>(alloc, device, render_pass);
    }

} // namespace render::vulkan

