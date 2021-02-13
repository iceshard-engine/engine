#include "vk_device.hxx"
#include "vk_swapchain.hxx"
#include "vk_render_surface.hxx"
#include "vk_image.hxx"
#include "vk_utility.hxx"
#include <ice/assert.hxx>

namespace ice::render::vk
{

    VulkanRenderDevice::VulkanRenderDevice(
        ice::Allocator& alloc,
        VkDevice vk_device,
        VkPhysicalDevice vk_physical_device,
        VkPhysicalDeviceMemoryProperties const& memory_properties
    ) noexcept
        : _allocator{ alloc }
        , _vk_device{ vk_device }
        , _vk_physical_device{ vk_physical_device }
        , _vk_memory_manager{ ice::make_unique<VulkanMemoryManager>(_allocator, _allocator, _vk_device, memory_properties) }
        , _vk_queues{ _allocator }
    {
    }

    VulkanRenderDevice::~VulkanRenderDevice() noexcept
    {
        _vk_memory_manager = nullptr;
        vkDestroyDevice(_vk_device, nullptr);
    }

    auto VulkanRenderDevice::create_swapchain(
        ice::render::RenderSurface* surface
    ) noexcept -> ice::render::RenderSwapchain*
    {
        ICE_ASSERT(surface != nullptr, "Cannot create swapchain for nullptr surface!");
        VulkanRenderSurface const* const vk_surface = static_cast<VulkanRenderSurface*>(surface);

        ice::pod::Array<VkSurfaceFormatKHR> surface_formats{ _allocator };
        ice::render::vk::enumerate_objects(
            surface_formats,
            vkGetPhysicalDeviceSurfaceFormatsKHR,
            _vk_physical_device,
            vk_surface->handle()
        );

        ICE_ASSERT(
            ice::pod::array::empty(surface_formats) == false,
            "No supported image formats found for given surface!"
        );

        VkSurfaceCapabilitiesKHR surface_capabilities;
        VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            _vk_physical_device,
            vk_surface->handle(),
            &surface_capabilities
        );

        ICE_ASSERT(
            result == VkResult::VK_SUCCESS,
            "Failed to query surface capabilities!"
        );

        VkSurfaceFormatKHR surface_format = ice::pod::array::front(surface_formats);

        VkSwapchainCreateInfoKHR swapchain_info{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        swapchain_info.surface = vk_surface->handle();
        swapchain_info.imageFormat = surface_format.format;
        swapchain_info.minImageCount = surface_capabilities.minImageCount;
        swapchain_info.imageExtent = surface_capabilities.maxImageExtent;
        swapchain_info.imageArrayLayers = 1;
        swapchain_info.oldSwapchain = VK_NULL_HANDLE;
        swapchain_info.clipped = false; // Clipped for android only?
        swapchain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        swapchain_info.imageColorSpace = surface_format.colorSpace;
        swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_info.queueFamilyIndexCount = 0;
        swapchain_info.pQueueFamilyIndices = nullptr;

        if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        {
            swapchain_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        }
        else
        {
            swapchain_info.preTransform = surface_capabilities.currentTransform;
        }

        // Find a supported composite alpha mode - one of these is guaranteed to be set
        VkCompositeAlphaFlagBitsKHR composite_alpha = VkCompositeAlphaFlagBitsKHR::VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        VkCompositeAlphaFlagBitsKHR composite_alpha_flags[4] = {
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
        };
        for (uint32_t i = 0; i < sizeof(composite_alpha_flags) / sizeof(composite_alpha_flags[0]); i++)
        {
            if (surface_capabilities.supportedCompositeAlpha & composite_alpha_flags[i])
            {
                swapchain_info.compositeAlpha = composite_alpha_flags[i];
                break;
            }
        } // Guaranteed to be suppoted

        //uint32_t queueFamilyIndices[2] = { (uint32_t)info.graphics_queue_family_index, (uint32_t)info.present_queue_family_index };
        //if (info.graphics_queue_family_index != info.present_queue_family_index)
        //{
        //    // If the graphics and present queues are from different queue families,
        //    // we either have to explicitly transfer ownership of images between the
        //    // queues, or we have to create the swapchain with imageSharingMode
        //    // as VK_SHARING_MODE_CONCURRENT
        //    swapchain_ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        //    swapchain_ci.queueFamilyIndexCount = 2;
        //    swapchain_ci.pQueueFamilyIndices = queueFamilyIndices;
        //}

        VkSwapchainKHR vk_swapchain;
        result = vkCreateSwapchainKHR(
            _vk_device,
            &swapchain_info,
            nullptr,
            &vk_swapchain
        );
        ICE_ASSERT(
            result == VkResult::VK_SUCCESS,
            "Couldn't create swapchain!"
        );

        return _allocator.make<VulkanSwapchain>(
            vk_swapchain,
            surface_format.format,
            swapchain_info.imageExtent,
            _vk_device
        );
    }

    void VulkanRenderDevice::destroy_swapchain(ice::render::RenderSwapchain* swapchain) noexcept
    {
        _allocator.destroy(static_cast<VulkanSwapchain*>(swapchain));
    }

    auto VulkanRenderDevice::create_renderpass(ice::render::RenderPassInfo const& info) noexcept -> ice::render::RenderPass
    {
        ice::pod::Array<VkAttachmentDescription> attachments{ _allocator };
        ice::pod::array::reserve(attachments, static_cast<ice::u32>(info.attachments.size()));

        ice::pod::Array<VkSubpassDescription> subpass_list{ _allocator };
        ice::pod::array::reserve(subpass_list, static_cast<ice::u32>(info.subpasses.size()));

        ice::pod::Array<VkSubpassDependency> dependencies { _allocator };
        ice::pod::array::reserve(dependencies, static_cast<ice::u32>(info.dependencies.size()));

        ice::pod::Array<VkAttachmentReference> attachment_references{ _allocator };


        for (RenderAttachment const& attachment_info : info.attachments)
        {
            VkAttachmentDescription attachment{ }; // VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2 };
            attachment.flags = 0;
            attachment.format = native_enum_value(attachment_info.format);
            attachment.samples = VK_SAMPLE_COUNT_1_BIT;

            for (AttachmentOperation op : attachment_info.operations)
            {
                native_enum_value(op, attachment.loadOp);
                native_enum_value(op, attachment.storeOp);
            }
            for (AttachmentOperation op : attachment_info.stencil_operations)
            {
                native_enum_value(op, attachment.stencilLoadOp);
                native_enum_value(op, attachment.stencilStoreOp);
            }

            attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachment.finalLayout = native_enum_value(attachment_info.layout);

            ice::pod::array::push_back(attachments, attachment);
        }

        ice::u64 reference_count = 0;
        for (RenderSubPass const& subpass_info : info.subpasses)
        {
             reference_count += subpass_info.color_attachments.size()
                + subpass_info.input_attachments.size()
                + 1;

        }
        ice::pod::array::reserve(attachment_references, static_cast<ice::u32>(reference_count));

        auto store_references = [&attachment_references](ice::Span<AttachmentReference const> references) noexcept -> ice::u32
        {
            ice::u32 ref_index = ice::pod::array::size(attachment_references);
            for (AttachmentReference const& attachment_ref : references)
            {
                VkAttachmentReference reference{ }; // VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2 };
                reference.attachment = attachment_ref.attachment_index;
                reference.layout = native_enum_value(attachment_ref.layout);

                ice::pod::array::push_back(attachment_references, reference);
            }
            return ref_index;
        };

        for (RenderSubPass const& subpass_info : info.subpasses)
        {
            ice::u32 const input_ref_idx = store_references(subpass_info.input_attachments);
            ice::u32 const color_ref_idx = store_references(subpass_info.color_attachments);

            VkSubpassDescription subpass{ }; // VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2 };
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.inputAttachmentCount = static_cast<ice::u32>(subpass_info.input_attachments.size());
            if (subpass.inputAttachmentCount > 0)
            {
                subpass.pInputAttachments = std::addressof(attachment_references[input_ref_idx]);
            }
            subpass.colorAttachmentCount = static_cast<ice::u32>(subpass_info.color_attachments.size());
            if (subpass.colorAttachmentCount > 0)
            {
                subpass.pColorAttachments = std::addressof(attachment_references[color_ref_idx]);
            }
            if (subpass_info.depth_stencil_attachment.layout != ImageLayout::Undefined)
            {
                ice::u32 const depth_ref_idx = store_references(ice::Span<AttachmentReference const>(&subpass_info.depth_stencil_attachment, 1llu));
                subpass.pDepthStencilAttachment = std::addressof(attachment_references[depth_ref_idx]);
            }

            ice::pod::array::push_back(subpass_list, subpass);
        }

        for (SubpassDependency const& dependency_info : info.dependencies)
        {
            VkSubpassDependency dependency{ VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2 };
            dependency.srcSubpass = dependency_info.source_subpass;
            dependency.srcStageMask = native_enum_value(dependency_info.source_stage);
            dependency.srcAccessMask = native_enum_value(dependency_info.source_access);
            dependency.dstSubpass = dependency_info.destination_subpass;
            dependency.dstStageMask = native_enum_value(dependency_info.destination_stage);
            dependency.dstAccessMask = native_enum_value(dependency_info.destination_access);

            ice::pod::array::push_back(dependencies, dependency);
        }

        VkRenderPassCreateInfo renderpass_info{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
        renderpass_info.attachmentCount = ice::pod::array::size(attachments);
        renderpass_info.pAttachments = ice::pod::array::begin(attachments);
        renderpass_info.subpassCount = ice::pod::array::size(subpass_list);
        renderpass_info.pSubpasses = ice::pod::array::begin(subpass_list);
        renderpass_info.dependencyCount = ice::pod::array::size(dependencies);
        renderpass_info.pDependencies = ice::pod::array::begin(dependencies);

        VkRenderPass renderpass;
        VkResult result = vkCreateRenderPass(_vk_device, &renderpass_info, nullptr, &renderpass);
        ICE_ASSERT(
            result == VkResult::VK_SUCCESS,
            "Couldn't create render pass with given description!"
        );

        return static_cast<RenderPass>(reinterpret_cast<ice::uptr>(renderpass));
    }

    void VulkanRenderDevice::destroy_renderpass(ice::render::RenderPass renderpass) noexcept
    {
        VkRenderPass vk_renderpass = reinterpret_cast<VkRenderPass>(static_cast<ice::uptr>(renderpass));
        vkDestroyRenderPass(_vk_device, vk_renderpass, nullptr);
    }

    auto VulkanRenderDevice::create_framebuffer(
        ice::vec2u extent,
        ice::render::RenderPass renderpass,
        ice::Span<ice::render::Image> images
    ) noexcept -> ice::render::Framebuffer
    {
        VkRenderPass vk_renderpass = reinterpret_cast<VkRenderPass>(static_cast<ice::uptr>(renderpass));

        ice::pod::Array<VkImageView> vk_images{ _allocator };
        ice::pod::array::reserve(vk_images, static_cast<ice::u32>(images.size()));

        for (Image image : images)
        {
            VulkanImage* const image_ptr = reinterpret_cast<VulkanImage*>(static_cast<ice::uptr>(image));
            ice::pod::array::push_back(vk_images, image_ptr->vk_image_view);
        }

        VkFramebufferCreateInfo fb_info{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        fb_info.renderPass = vk_renderpass;
        fb_info.attachmentCount = ice::pod::array::size(vk_images);
        fb_info.pAttachments = ice::pod::array::begin(vk_images);
        fb_info.width = extent.x;
        fb_info.height = extent.y;
        fb_info.layers = 1;

        VkFramebuffer vk_framebuffer;
        VkResult result = vkCreateFramebuffer(_vk_device, &fb_info, nullptr, &vk_framebuffer);
        ICE_ASSERT(
            result == VkResult::VK_SUCCESS,
            "Couldn't create framebuffer with given images!"
        );

        return static_cast<Framebuffer>(reinterpret_cast<ice::uptr>(vk_framebuffer));
    }

    void VulkanRenderDevice::destroy_framebuffer(
        ice::render::Framebuffer framebuffer
    ) noexcept
    {
        VkFramebuffer vk_framebuffer = reinterpret_cast<VkFramebuffer>(static_cast<ice::uptr>(framebuffer));
        vkDestroyFramebuffer(_vk_device, vk_framebuffer, nullptr);
    }

    auto VulkanRenderDevice::create_image(ice::render::ImageInfo image, ice::Data data) noexcept -> ice::render::Image
    {
        VkImageCreateInfo image_info{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.format = native_enum_value(image.format);
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.extent.width = image.width;
        image_info.extent.height = image.height;
        image_info.extent.depth = 1;
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.usage = native_enum_flags(image.usage);
        image_info.queueFamilyIndexCount = 0;
        image_info.pQueueFamilyIndices = nullptr;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkImage vk_image;
        VkResult result = vkCreateImage(_vk_device, &image_info, nullptr, &vk_image);
        ICE_ASSERT(
            result == VkResult::VK_SUCCESS,
            "Couldn't create image object!"
        );

        AllocationInfo memory_info;
        AllocationHandle memory_handle = _vk_memory_manager->allocate(
            vk_image,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            AllocationType::RenderTarget,
            memory_info
        );

        VkImageViewCreateInfo view_info{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        view_info.image = vk_image;
        view_info.format = image_info.format;
        view_info.components.r = VK_COMPONENT_SWIZZLE_R;
        view_info.components.g = VK_COMPONENT_SWIZZLE_G;
        view_info.components.b = VK_COMPONENT_SWIZZLE_B;
        view_info.components.a = VK_COMPONENT_SWIZZLE_A;
        if (image_info.format == VK_FORMAT_D16_UNORM
            || image_info.format == VK_FORMAT_D32_SFLOAT
            || image_info.format == VK_FORMAT_D16_UNORM_S8_UINT
            || image_info.format == VK_FORMAT_D24_UNORM_S8_UINT
            || image_info.format == VK_FORMAT_D32_SFLOAT_S8_UINT)
        {
            view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        else
        {
            view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;

        VkImageView vk_image_view;
        result = vkCreateImageView(_vk_device, &view_info, nullptr, &vk_image_view);
        ICE_ASSERT(
            result == VkResult::VK_SUCCESS,
            "Coudln't create image view!"
        );

        VulkanImage* const image_ptr = _allocator.make<VulkanImage>(vk_image, vk_image_view, memory_handle);
        return static_cast<Image>(reinterpret_cast<ice::uptr>(image_ptr));
    }

    void VulkanRenderDevice::destroy_image(ice::render::Image image) noexcept
    {
        VulkanImage* const image_ptr = reinterpret_cast<VulkanImage*>(static_cast<ice::uptr>(image));
        vkDestroyImageView(_vk_device, image_ptr->vk_image_view, nullptr);
        _vk_memory_manager->release(image_ptr->vk_alloc_handle);
        vkDestroyImage(_vk_device, image_ptr->vk_image, nullptr);
        _allocator.destroy(image_ptr);
    }

    /*auto VulkanRenderDevice::queue_count() const noexcept -> ice::u32
    {
        return ice::pod::array::size(_vk_queues);
    }*/

    auto VulkanRenderDevice::create_queue(
        ice::render::QueueID queue_id,
        ice::u32 queue_index,
        ice::u32 command_pools
    ) const noexcept -> ice::render::RenderQueue*
    {
        ice::u32 const queue_family_index = static_cast<ice::u32>(queue_id);

        VkQueue queue = vk_nullptr;
        vkGetDeviceQueue(_vk_device, queue_family_index, queue_index, &queue);

        ice::pod::Array<VkCommandPool> cmd_pools{ _allocator };
        ice::pod::array::reserve(cmd_pools, command_pools);

        for (ice::u32 idx = 0; idx < command_pools; ++idx)
        {
            VkCommandPoolCreateInfo cmd_pool_info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
            cmd_pool_info.queueFamilyIndex = queue_family_index;
            cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

            VkCommandPool vk_cmd_pool;
            VkResult result = vkCreateCommandPool(_vk_device, &cmd_pool_info, nullptr, &vk_cmd_pool);
            ICE_ASSERT(
                result == VkResult::VK_SUCCESS,
                "Failed to create command pool for device!"
            );

            ice::pod::array::push_back(cmd_pools, vk_cmd_pool);
        }

        return _allocator.make<VulkanQueue>(queue, _vk_device, ice::move(cmd_pools));
    }

    void VulkanRenderDevice::destroy_queue(ice::render::RenderQueue* queue) const noexcept
    {
        _allocator.destroy(static_cast<VulkanQueue*>(queue));
    }

    auto VulkanRenderDevice::get_commands() noexcept -> ice::render::RenderCommands&
    {
        return _vk_render_commands;
    }

    auto native_handle(CommandBuffer cmds) noexcept
    {
        return reinterpret_cast<VkCommandBuffer>(static_cast<ice::uptr>(cmds));
    }

    auto native_handle(RenderPass renderpass) noexcept
    {
        return reinterpret_cast<VkRenderPass>(static_cast<ice::uptr>(renderpass));
    }

    auto native_handle(Framebuffer framebuffer) noexcept
    {
        return reinterpret_cast<VkFramebuffer>(static_cast<ice::uptr>(framebuffer));
    }

    void VulkanRenderCommands::begin(
        ice::render::CommandBuffer cmds
    ) noexcept
    {
        VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        vkBeginCommandBuffer(
            native_handle(cmds),
            &begin_info
        );
    }

    void VulkanRenderCommands::begin_renderpass(
        ice::render::CommandBuffer cmds,
        ice::render::RenderPass renderpass,
        ice::render::Framebuffer framebuffer,
        ice::vec2u extent,
        ice::vec4f clear_color
    ) noexcept
    {
        VkClearValue clear_values[3];
        clear_values[0].color.float32[0] = clear_color.x;
        clear_values[0].color.float32[1] = clear_color.y;
        clear_values[0].color.float32[2] = clear_color.z;
        clear_values[0].color.float32[3] = clear_color.w;
        //clear_values[1] = clear_values[0];
        clear_values[1].depthStencil.depth = 1.0f;
        clear_values[1].depthStencil.stencil = 0;

        VkRenderPassBeginInfo begin_info{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        begin_info.renderPass = native_handle(renderpass);
        begin_info.framebuffer = native_handle(framebuffer);
        begin_info.renderArea.offset.x = 0;
        begin_info.renderArea.offset.y = 0;
        begin_info.renderArea.extent = { extent.x, extent.y };
        begin_info.clearValueCount = 2;
        begin_info.pClearValues = clear_values;

        vkCmdBeginRenderPass(
            native_handle(cmds),
            &begin_info,
            VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE
        );
    }

    void VulkanRenderCommands::next_subpass(
        ice::render::CommandBuffer cmds,
        ice::render::SubPassContents contents
    ) noexcept
    {
        vkCmdNextSubpass(
            native_handle(cmds),
            contents == SubPassContents::Inline
                ? VK_SUBPASS_CONTENTS_INLINE
                : VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
        );
    }

    void VulkanRenderCommands::end_renderpass(
        ice::render::CommandBuffer cmds
    ) noexcept
    {
        vkCmdEndRenderPass(
            native_handle(cmds)
        );
    }

    void VulkanRenderCommands::end(
        ice::render::CommandBuffer cmds
    ) noexcept
    {
        vkEndCommandBuffer(
            native_handle(cmds)
        );
    }

} // namespace ice::render::vk
