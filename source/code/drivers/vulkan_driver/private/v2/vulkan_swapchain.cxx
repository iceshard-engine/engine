#include <iceshard/renderer/vulkan/vulkan_swapchain.hxx>

namespace iceshard::renderer::vulkan
{

    namespace detail
    {

        class VulkanSwapchainObject final
        {
        public:
            VulkanSwapchainObject(
                core::allocator& alloc,
                VulkanDevices devices,
                VkSwapchainKHR swapchain,
                core::pod::Array<VulkanSwapchainImage>&& images
            ) noexcept
                : _allocator{ alloc }
                , _vk_device{ devices.graphics.handle }
                , _vk_swapchain{ swapchain }
                , _swapchain_images{ std::move(images) }
            {
            }

            ~VulkanSwapchainObject() noexcept
            {
                for (auto image_info : _swapchain_images)
                {
                    vkDestroyImageView(_vk_device, image_info.view, nullptr);
                }
                vkDestroySwapchainKHR(_vk_device, _vk_swapchain, nullptr);
            }

            auto native_handle() noexcept -> VkSwapchainKHR
            {
                return _vk_swapchain;
            }

            auto swapchain_images() noexcept -> core::pod::Array<VulkanSwapchainImage> const&
            {
                return _swapchain_images;
            }

        private:
            core::allocator& _allocator;
            VkDevice const _vk_device;
            VkSwapchainKHR const _vk_swapchain;
            core::pod::Array<VulkanSwapchainImage> _swapchain_images;
        };

        union VulkanSwapchainHandle
        {
            VulkanSwapchain handle;
            VulkanSwapchainObject* object;
        };

    } // namespace detail

    auto swapchain_images(VulkanSwapchain swapchain) noexcept -> core::pod::Array<VulkanSwapchainImage> const&
    {
        return detail::VulkanSwapchainHandle{ swapchain }.object->swapchain_images();
    }

    auto native_handle(VulkanSwapchain swapchain) noexcept -> VkSwapchainKHR
    {
        return detail::VulkanSwapchainHandle{ swapchain }.object->native_handle();
    }

    auto create_swapchain(
        core::allocator& alloc,
        VulkanDevices devices,
        VulkanSurface surface
    ) noexcept -> VulkanSwapchain
    {
        VkSurfaceFormatKHR format;
        VkSurfaceCapabilitiesKHR capabilities;

        get_surface_format(devices.physical.handle, surface, format);
        get_surface_capabilities(devices.physical.handle, surface, capabilities);

        VkSwapchainCreateInfoKHR swapchain_ci = {};

        swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchain_ci.pNext = NULL;
        swapchain_ci.surface = native_handle(surface);
        swapchain_ci.imageFormat = format.format;

        uint32_t swapchain_image_count = capabilities.minImageCount;

        VkSurfaceTransformFlagBitsKHR pre_transform;
        if (capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        {
            pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        }
        else
        {
            pre_transform = capabilities.currentTransform;
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
            if (capabilities.supportedCompositeAlpha & composite_alpha_flags[i])
            {
                composite_alpha = composite_alpha_flags[i];
                break;
            }
        }

        swapchain_ci.minImageCount = swapchain_image_count;
        swapchain_ci.imageExtent = capabilities.maxImageExtent;
        swapchain_ci.preTransform = pre_transform;
        swapchain_ci.compositeAlpha = composite_alpha;
        swapchain_ci.imageArrayLayers = 1;
        swapchain_ci.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR; // Guaranteed to be suppoted
        swapchain_ci.oldSwapchain = VK_NULL_HANDLE;
        swapchain_ci.clipped = false; // Clipped for android only?

        swapchain_ci.imageColorSpace = format.colorSpace;
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
        auto api_result = vkCreateSwapchainKHR(devices.graphics.handle, &swapchain_ci, nullptr, &swapchain_handle);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create swapchain!");

        core::pod::Array<VulkanSwapchainImage> swapchain_images{ alloc };

        {
            swapchain_image_count = 0;

            api_result = vkGetSwapchainImagesKHR(
                devices.graphics.handle,
                swapchain_handle,
                &swapchain_image_count,
                nullptr
            );
            IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't query swapchain image count!");

            core::pod::Array<VkImage> swapchain_image_handles{ alloc };
            core::pod::array::resize(swapchain_image_handles, swapchain_image_count);

            api_result = vkGetSwapchainImagesKHR(
                devices.graphics.handle,
                swapchain_handle,
                &swapchain_image_count,
                core::pod::array::begin(swapchain_image_handles)
            );
            IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't query swapchain images!");

            for (auto image_handle : swapchain_image_handles)
            {
                VkImageViewCreateInfo color_image_view = {};
                color_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                color_image_view.pNext = NULL;
                color_image_view.flags = 0;
                color_image_view.image = image_handle;
                color_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
                color_image_view.format = format.format;
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
                api_result = vkCreateImageView(devices.graphics.handle, &color_image_view, nullptr, &image_view_handle);
                IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create view for swapchain image!");

                // Save both handles.
                core::pod::array::push_back(swapchain_images, { image_handle, image_view_handle });
            }
        }

        detail::VulkanSwapchainHandle handle{ };
        handle.object = alloc.make<detail::VulkanSwapchainObject>(
            alloc,
            devices,
            swapchain_handle,
            std::move(swapchain_images)
        );
        return handle.handle;
    }

    void destroy_swapchain(
        core::allocator& alloc,
        VulkanSwapchain swapchain
    ) noexcept
    {
        detail::VulkanSwapchainHandle handle{ swapchain };
        alloc.destroy(handle.object);
    }

} // namespace iceshard::renderer::vulkan
