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
        VulkanImage const* postprocess_image,
        VulkanPhysicalDevice const* vulkan_physical_device
    ) noexcept
    {
        if (postprocess_image == nullptr)
        {
            VkImageView attachments[2];
            attachments[1] = depth_buffer.native_view();

            auto surface_capabilities = vulkan_physical_device->surface_capabilities();

            VkFramebufferCreateInfo fb_info = {};
            fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fb_info.pNext = nullptr;
            fb_info.renderPass = render_pass;
            fb_info.attachmentCount = 2;
            fb_info.pAttachments = attachments;
            fb_info.width = surface_capabilities.maxImageExtent.width;
            fb_info.height = surface_capabilities.maxImageExtent.height;
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
        else
        {
            VkImageView attachments[3];
            attachments[0] = postprocess_image->native_view();
            attachments[2] = depth_buffer.native_view();

            auto surface_capabilities = vulkan_physical_device->surface_capabilities();

            VkFramebufferCreateInfo fb_info = {};
            fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fb_info.pNext = nullptr;
            fb_info.renderPass = render_pass;
            fb_info.attachmentCount = 3;
            fb_info.pAttachments = attachments;
            fb_info.width = surface_capabilities.maxImageExtent.width;
            fb_info.height = surface_capabilities.maxImageExtent.height;
            fb_info.layers = 1;

            for (auto const& swapchain_buffer : swapchain.swapchain_buffers())
            {
                attachments[1] = swapchain_buffer.view;

                VkFramebuffer framebuffer;
                auto api_result = vkCreateFramebuffer(device, &fb_info, nullptr, &framebuffer);
                IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create framebuffer object!");

                core::pod::array::push_back(results, alloc.make<VulkanFramebuffer>(device, framebuffer));
            }
        }
    }

} // namespace render::vulkan
