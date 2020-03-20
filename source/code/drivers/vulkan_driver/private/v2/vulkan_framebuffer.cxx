#include <iceshard/renderer/vulkan/vulkan_framebuffer.hxx>
#include <core/allocators/stack_allocator.hxx>

#include "framebuffer/vulkan_framebuffer_image.hxx"

namespace iceshard::renderer::vulkan
{

    namespace detail
    {

        class VulkanFramebufferObject final
        {
        public:
            VulkanFramebufferObject(
                VkDevice device,
                VkFramebuffer framebuffer,
                core::pod::Array<VulkanFramebufferImage>&& images
            ) noexcept
                : _vk_device{ device }
                , _vk_framebuffer{ framebuffer }
                , _images{ std::move(images) }
            {
            }

            auto get_image_view(uint32_t index) noexcept -> VkImageView
            {
                return _images[index].image_view;
            }

            auto native_handle() noexcept
            {
                return _vk_framebuffer;
            }

            ~VulkanFramebufferObject() noexcept
            {
                for (auto const& image : _images)
                {
                    vkDestroyImageView(
                        _vk_device,
                        image.image_view,
                        nullptr
                    );
                    vkFreeMemory(
                        _vk_device,
                        image.allocated_image_memory,
                        nullptr
                    );
                    vkDestroyImage(
                        _vk_device,
                        image.allocated_image,
                        nullptr
                    );
                }
                vkDestroyFramebuffer(
                    _vk_device,
                    _vk_framebuffer,
                    nullptr
                );
            }

        private:
            VkDevice const _vk_device;
            VkFramebuffer const _vk_framebuffer;
            core::pod::Array<VulkanFramebufferImage> _images;
        };

        union VulkanFramebufferHandle
        {
            VulkanFramebuffer framebuffer;
            VulkanFramebufferObject* object;
        };

    } // namespace detail

    auto framebuffer_texture_from_handle(iceshard::renderer::api::Texture handle) noexcept -> VulkanFramebufferTexture
    {
        switch (handle)
        {
        case iceshard::renderer::api::v1_1::types::Texture::Invalid:
            break;
        case iceshard::renderer::api::v1_1::types::Texture::Attachment0:
            return VulkanFramebufferTexture::Attachment0;
        case iceshard::renderer::api::v1_1::types::Texture::Attachment1:
            return VulkanFramebufferTexture::Attachment1;
        case iceshard::renderer::api::v1_1::types::Texture::Attachment2:
            return VulkanFramebufferTexture::Attachment2;
        case iceshard::renderer::api::v1_1::types::Texture::Attachment3:
            return VulkanFramebufferTexture::Attachment3;
        }
        IS_ASSERT(false, "Invalid enum value!");
        std::abort();
    }

    auto framebuffer_image(VulkanFramebuffer framebuffer, VulkanFramebufferTexture texture) noexcept -> VkImageView
    {
        return detail::VulkanFramebufferHandle{ framebuffer }.object->get_image_view(static_cast<uint32_t>(texture));
    }

    auto native_handle(VulkanFramebuffer framebuffer) noexcept -> VkFramebuffer
    {
        return detail::VulkanFramebufferHandle{ framebuffer }.object->native_handle();
    }

    void create_framebuffers(
        core::allocator& alloc,
        VkExtent2D framebuffer_extent,
        VulkanDevices devices,
        VulkanRenderPass renderpass,
        VulkanSwapchain swapchain,
        core::pod::Array<VulkanFramebuffer>& framebuffer_results
    ) noexcept
    {
        core::memory::stack_allocator_512 temp_alloc;

        int32_t swapchain_image_index = -1;
        core::pod::Array<VulkanFramebufferImage> framebuffer_images{ alloc };

        uint32_t renderpass_image_count = 0;
        {
            core::pod::Array<RenderPassImage> renderpass_images{ temp_alloc };
            get_renderpass_image_info(renderpass, renderpass_images);

            renderpass_image_count = core::pod::array::size(renderpass_images);

            // Define the framebuffer image array.
            int32_t current_image_index = 0;
            for (auto const& rp_image : renderpass_images)
            {
                if (rp_image.type == RenderPassImageType::DepthStencilImage)
                {
                    // Create dept-stencil image.
                    core::pod::array::push_back(
                        framebuffer_images,
                        create_framebuffer_image(
                            devices,
                            framebuffer_extent,
                            rp_image.format,
                            VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                        )
                    );
                }
                else if (rp_image.type == RenderPassImageType::ColorImage)
                {
                    // Create dept-stencil image.
                    core::pod::array::push_back(
                        framebuffer_images,
                        create_framebuffer_image(
                            devices,
                            framebuffer_extent,
                            rp_image.format,
                            VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                            | VkImageUsageFlagBits::VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
                            | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT
                        )
                    );
                }
                else if (rp_image.type == RenderPassImageType::SwapchainImage)
                {
                    IS_ASSERT(swapchain_image_index == -1, "A swapchain image can be used only once in a framebuffer!");
                    swapchain_image_index = current_image_index;
                }

                current_image_index += 1;
            }

            temp_alloc.clear();
        }

        // Create and fill the attachments array.
        core::pod::Array<VkImageView> attachments{ temp_alloc };
        core::pod::array::resize(attachments, renderpass_image_count);

        uint32_t framebuffer_image_idx = 0;
        for (int32_t attachment_idx = 0; attachment_idx < static_cast<int32_t>(renderpass_image_count); ++attachment_idx)
        {
            if (attachment_idx != swapchain_image_index)
            {
                attachments[attachment_idx] = framebuffer_images[framebuffer_image_idx].image_view;
                framebuffer_image_idx += 1;
            }
        }

        VkFramebufferCreateInfo fb_info = {};
        fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb_info.pNext = nullptr;
        fb_info.renderPass = renderpass.renderpass;
        fb_info.attachmentCount = core::pod::array::size(attachments);
        fb_info.pAttachments = core::pod::array::begin(attachments);
        fb_info.width = framebuffer_extent.width;
        fb_info.height = framebuffer_extent.height;
        fb_info.layers = 1;

        IS_ASSERT(swapchain_image_index >= 0, "Framebuffers without swapchain images are not supported yet!");
        for (auto const& swapchain_image : iceshard::renderer::vulkan::swapchain_images(swapchain))
        {
            attachments[swapchain_image_index] = swapchain_image.view;

            VkFramebuffer framebuffer;
            auto api_result = vkCreateFramebuffer(devices.graphics.handle, &fb_info, nullptr, &framebuffer);
            IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create framebuffer object!");

            detail::VulkanFramebufferHandle handle;
            handle.object = alloc.make<detail::VulkanFramebufferObject>(
                devices.graphics.handle,
                framebuffer,
                std::move(framebuffer_images)
            );

            core::pod::array::push_back(
                framebuffer_results,
                handle.framebuffer
            );
        }
    }

    void destroy_framebuffers(
        core::allocator& alloc,
        core::pod::Array<VulkanFramebuffer>& framebuffers
    ) noexcept
    {
        for (auto const& framebuffer : framebuffers)
        {
            detail::VulkanFramebufferHandle handle{ framebuffer };
            alloc.destroy(handle.object);
        }
        core::pod::array::clear(framebuffers);
    }

} // namespace iceshard::renderer::vulkan
