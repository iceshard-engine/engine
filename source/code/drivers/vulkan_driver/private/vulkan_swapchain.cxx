#include "vulkan_swapchain.hxx"

namespace render::vulkan
{

    VulkanSwapchain::VulkanSwapchain(core::allocator& alloc, VkDevice device, VkFormat format, VkSwapchainKHR swapchain) noexcept
        : _allocator{ alloc }
        , _graphics_device{ device }
        , _surface_format{ format }
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
        uint32_t swapchain_image_count;
        auto api_result = vkGetSwapchainImagesKHR(_graphics_device, _swapchain_handle, &swapchain_image_count, NULL);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't get number of swapchain images!");

        core::pod::Array<VkImage> swapchain_images{ _allocator };
        core::pod::array::resize(swapchain_images, swapchain_image_count);

        api_result = vkGetSwapchainImagesKHR(_graphics_device, _swapchain_handle, &swapchain_image_count, &swapchain_images[0]);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't query swapchain images!");

        for (auto image_handle : swapchain_images)
        {
            VkImageViewCreateInfo color_image_view = {};
            color_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            color_image_view.pNext = NULL;
            color_image_view.flags = 0;
            color_image_view.image = image_handle;
            color_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
            color_image_view.format = _surface_format;
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
            api_result = vkCreateImageView(_graphics_device, &color_image_view, NULL, &image_view_handle);
            IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create view for swapchain image!");

            core::pod::array::push_back(_swapchain_buffers, SwapchainBuffer{ image_handle, image_view_handle });
        }
    }

    void VulkanSwapchain::shutdown() noexcept
    {
        for (auto swapchain_buffer : _swapchain_buffers)
        {
            vkDestroyImageView(_graphics_device, swapchain_buffer.view, NULL);
        }
        vkDestroySwapchainKHR(_graphics_device, _swapchain_handle, nullptr);
    }

    auto create_swapchain(
        core::allocator& alloc,
        VkPhysicalDevice physical_device,
        VkDevice graphics_device,
        VkSurfaceKHR surface
    ) noexcept -> core::memory::unique_pointer<VulkanSwapchain>
    {
        VkSurfaceFormatKHR surface_format;
        VkSurfaceCapabilitiesKHR surface_capabilities;

        // Swapchain surface image format
        {
            uint32_t surface_format_number;
            VkResult api_result = vkGetPhysicalDeviceSurfaceFormatsKHR(
                physical_device, surface,
                &surface_format_number, nullptr
            );
            IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't get number of device surface formats!");

            core::pod::Array<VkSurfaceFormatKHR> surface_formats{ alloc };
            core::pod::array::resize(surface_formats, surface_format_number);

            api_result = vkGetPhysicalDeviceSurfaceFormatsKHR(
                physical_device,
                surface,
                &surface_format_number, core::pod::array::begin(surface_formats)
            );
            IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't query device surface formats!");
            surface_format = core::pod::array::front(surface_formats);
        }

        {
            auto api_result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                physical_device,
                surface,
                &surface_capabilities
            );
            IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't get device surface capabilities!");
        }

        VkSwapchainCreateInfoKHR swapchain_ci = {};

        swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchain_ci.pNext = NULL;
        swapchain_ci.surface = surface;
        swapchain_ci.imageFormat = surface_format.format;

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

        VkSwapchainKHR swapchain_handle;
        auto api_result = vkCreateSwapchainKHR(graphics_device, &swapchain_ci, NULL, &swapchain_handle);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create swapchain!");

        return core::memory::make_unique<VulkanSwapchain>(alloc, alloc, graphics_device, surface_format.format, swapchain_handle);
    }

} // namespace render::vulkan
