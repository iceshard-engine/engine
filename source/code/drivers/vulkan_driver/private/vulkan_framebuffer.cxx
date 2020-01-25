#include "vulkan_framebuffer.hxx"

namespace render::vulkan
{

    VulkanFramebuffer::VulkanFramebuffer(VkDevice device, VkFramebuffer framebuffer_handle) noexcept
        : _device_handle{ device }
        , _native_handle{ framebuffer_handle }
    {
    }

    VulkanFramebuffer::~VulkanFramebuffer() noexcept
    {
        vkDestroyFramebuffer(_device_handle, _native_handle, nullptr);
    }

    void create_framebuffers(
        core::allocator& alloc,
        core::pod::Array<VulkanFramebuffer*>& results,
        VkDevice device,
        VkRenderPass render_pass,
        VulkanImage const& depth_buffer,
        VulkanSwapchain const& swapchain,
        VkExtent2D extent
    ) noexcept
    {
        VkImageView attachments[2];
        attachments[1] = depth_buffer.native_view();

        VkFramebufferCreateInfo fb_info = {};
        fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb_info.pNext = nullptr;
        fb_info.renderPass = render_pass;
        fb_info.attachmentCount = 2;
        fb_info.pAttachments = attachments;
        fb_info.width = extent.width;
        fb_info.height = extent.height;
        fb_info.layers = 1;

        for (auto const& swapchain_buffer : swapchain.swapchain_buffers())
        {
            attachments[0] = swapchain_buffer.view;

            VkFramebuffer framebuffer;
            auto api_result = vkCreateFramebuffer(device, &fb_info, nullptr, &framebuffer);
            IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create framebuffer object!");

            core::pod::array::push_back(results, alloc.make<VulkanFramebuffer>(device, framebuffer));
        }
    }

} // namespace render::vulkan
