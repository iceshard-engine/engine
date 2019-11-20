#include "vulkan_swapchain.hxx"

namespace render::vulkan
{

    VulkanSwapchain::VulkanSwapchain(VulkanDevice* vulkan_device, VkSwapchainKHR swapchain) noexcept
        : _vulkan_device{ vulkan_device }
        , _swapchain_handle{ swapchain }
    {
    }

    VulkanSwapchain::~VulkanSwapchain() noexcept
    {
        vkDestroySwapchainKHR(_vulkan_device->native_handle(), _swapchain_handle, nullptr);
    }

    auto create_swapchain(core::allocator& alloc, VulkanPhysicalDevice* vulkan_physical_device) noexcept -> core::memory::unique_pointer<VulkanSwapchain>
    {
        VkSwapchainCreateInfoKHR swapchain_ci = {};

        swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchain_ci.pNext = NULL;
        swapchain_ci.surface = vulkan_physical_device->surface_handle();

        auto const& surface_format = core::pod::array::front(vulkan_physical_device->surface_surface_formats());
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
        VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
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
        //swapchain_ci.imageUsage = VkImageUsageFlags:: usageFlags; // todo
        swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_ci.queueFamilyIndexCount = 0;
        swapchain_ci.pQueueFamilyIndices = NULL;

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

        //api_result = vkGetSwapchainImagesKHR(info.device, info.swap_chain, &info.swapchainImageCount, NULL);
        //IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't query swapchain images!");
        //assert(res == VK_SUCCESS);

        //VkImage* swapchainImages = (VkImage*)malloc(info.swapchainImageCount * sizeof(VkImage));
        //assert(swapchainImages);
        //res = vkGetSwapchainImagesKHR(info.device, info.swap_chain, &info.swapchainImageCount, swapchainImages);
        //assert(res == VK_SUCCESS);

        return core::memory::make_unique<VulkanSwapchain>(alloc, vulkan_device, swapchain_handle);
    }

} // namespace render::vulkan
