#include "vulkan_swapchain.hxx"

namespace render::vulkan
{

    VulkanSwapchain::VulkanSwapchain(core::allocator& alloc, VulkanPhysicalDevice* vulkan_device, VkSwapchainKHR swapchain) noexcept
        : _allocator{ alloc }
        , _vulkan_physical_device{ vulkan_device }
        , _swapchain_handle{ swapchain }
        , _swapchain_buffers{ _allocator }
    {
        initialize();
    }

    VulkanSwapchain::~VulkanSwapchain() noexcept
    {
        shutdown();
    }

    void VulkanSwapchain::initialize() noexcept
    {
        auto graphics_device = _vulkan_physical_device->graphics_device()->native_handle();

        uint32_t swapchain_image_count;
        auto api_result = vkGetSwapchainImagesKHR(graphics_device, _swapchain_handle, &swapchain_image_count, NULL);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't get number of swapchain images!");

        core::pod::Array<VkImage> swapchain_images{ _allocator };
        core::pod::array::resize(swapchain_images, swapchain_image_count);

        api_result = vkGetSwapchainImagesKHR(graphics_device, _swapchain_handle, &swapchain_image_count, &swapchain_images[0]);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't query swapchain images!");

        for (auto image_handle : swapchain_images)
        {
            VkImageViewCreateInfo color_image_view = {};
            color_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            color_image_view.pNext = NULL;
            color_image_view.flags = 0;
            color_image_view.image = image_handle;
            color_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
            color_image_view.format = core::pod::array::front(_vulkan_physical_device->surface_formats()).format;
            color_image_view.components.r = VK_COMPONENT_SWIZZLE_R;
            color_image_view.components.g = VK_COMPONENT_SWIZZLE_G;
            color_image_view.components.b = VK_COMPONENT_SWIZZLE_B;
            color_image_view.components.a = VK_COMPONENT_SWIZZLE_A;
            color_image_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            color_image_view.subresourceRange.baseMipLevel = 0;
            color_image_view.subresourceRange.levelCount = 1;
            color_image_view.subresourceRange.baseArrayLayer = 0;
            color_image_view.subresourceRange.layerCount = 1;

            VkImageView image_view_handle;
            api_result = vkCreateImageView(graphics_device, &color_image_view, NULL, &image_view_handle);
            IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create view for swapchain image!");

            core::pod::array::push_back(_swapchain_buffers, SwapchainBuffer{ image_handle, image_view_handle });
        }
    }

    void VulkanSwapchain::shutdown() noexcept
    {
        auto graphics_device = _vulkan_physical_device->graphics_device()->native_handle();

        for (auto swapchain_buffer : _swapchain_buffers)
        {
            vkDestroyImageView(graphics_device, swapchain_buffer.view, NULL);
        }
        vkDestroySwapchainKHR(graphics_device, _swapchain_handle, nullptr);
    }

    auto create_swapchain(core::allocator& alloc, VulkanPhysicalDevice* vulkan_physical_device) noexcept -> core::memory::unique_pointer<VulkanSwapchain>
    {
        VkSwapchainCreateInfoKHR swapchain_ci = {};

        swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchain_ci.pNext = NULL;
        swapchain_ci.surface = vulkan_physical_device->surface_handle();

        auto const& surface_format = core::pod::array::front(vulkan_physical_device->surface_formats());
        swapchain_ci.imageFormat = surface_format.format;

        auto const& surface_capabilities = vulkan_physical_device->surface_capabilities();

        uint32_t desiredNumberOfSwapChainImages = surface_capabilities.minImageCount;

        VkSurfaceTransformFlagBitsKHR preTransform;
        if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        {
            preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        }
        else
        {
            preTransform = surface_capabilities.currentTransform;
        }

        // Find a supported composite alpha mode - one of these is guaranteed to be set
        VkCompositeAlphaFlagBitsKHR compositeAlpha = VkCompositeAlphaFlagBitsKHR::VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        VkCompositeAlphaFlagBitsKHR compositeAlphaFlags[4] = {
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
        };
        for (uint32_t i = 0; i < sizeof(compositeAlphaFlags) / sizeof(compositeAlphaFlags[0]); i++)
        {
            if (surface_capabilities.supportedCompositeAlpha & compositeAlphaFlags[i])
            {
                compositeAlpha = compositeAlphaFlags[i];
                break;
            }
        }

        swapchain_ci.minImageCount = desiredNumberOfSwapChainImages;
        swapchain_ci.imageExtent = surface_capabilities.maxImageExtent;
        swapchain_ci.preTransform = preTransform;
        swapchain_ci.compositeAlpha = compositeAlpha;
        swapchain_ci.imageArrayLayers = 1;
        swapchain_ci.presentMode = VK_PRESENT_MODE_FIFO_KHR; // Guaranteed to be suppoted
        swapchain_ci.oldSwapchain = VK_NULL_HANDLE;
        swapchain_ci.clipped = false; // Clipped for android only?

        swapchain_ci.imageColorSpace = surface_format.colorSpace;
        swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // todo
        swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_ci.queueFamilyIndexCount = 0;
        swapchain_ci.pQueueFamilyIndices = nullptr;

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

        auto* vulkan_device = vulkan_physical_device->graphics_device();

        VkSwapchainKHR swapchain_handle;
        auto api_result = vkCreateSwapchainKHR(vulkan_device->native_handle(), &swapchain_ci, NULL, &swapchain_handle);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create swapchain!");

        return core::memory::make_unique<VulkanSwapchain>(alloc, alloc, vulkan_physical_device, swapchain_handle);
    }

} // namespace render::vulkan
