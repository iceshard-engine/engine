#include "vulkan_framebuffer_image.hxx"

namespace iceshard::renderer::vulkan
{

    auto create_framebuffer_image(
        VulkanDevices devices,
        VkExtent2D extent,
        VkFormat format,
        VkImageUsageFlags usage
    ) noexcept -> VulkanFramebufferImage
    {
        auto device = devices.graphics.handle;

        VulkanFramebufferImage result;

        VkImageCreateInfo image_info = {};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.pNext = nullptr;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.format = format;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.extent.width = extent.width;
        image_info.extent.height = extent.height;
        image_info.extent.depth = 1;
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.usage = usage;
        image_info.queueFamilyIndexCount = 0;
        image_info.pQueueFamilyIndices = nullptr;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.flags = 0;

        auto api_result = vkCreateImage(device, &image_info, nullptr, &result.allocated_image);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create depth buffer image!");

        VkMemoryRequirements memory_requirements;
        vkGetImageMemoryRequirements(device, result.allocated_image, &memory_requirements);

        VkMemoryAllocateInfo alloc_info{ };
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.allocationSize = memory_requirements.size;

        bool found_memory_type = find_memory_type_index(
            devices,
            memory_requirements,
            VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            alloc_info.memoryTypeIndex
        );
        IS_ASSERT(found_memory_type, "Couldn't find proper memory type for framebuffer image!");

        api_result = vkAllocateMemory(device, &alloc_info, nullptr, &result.allocated_image_memory);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't allocate memory for framebuffer image!");

        api_result = vkBindImageMemory(device, result.allocated_image, result.allocated_image_memory, 0);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't bind memory to framebuffer image!");

        VkImageViewCreateInfo view_info = {};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.pNext = nullptr;
        view_info.image = result.allocated_image;
        view_info.format = image_info.format;
        view_info.components.r = VK_COMPONENT_SWIZZLE_R;
        view_info.components.g = VK_COMPONENT_SWIZZLE_G;
        view_info.components.b = VK_COMPONENT_SWIZZLE_B;
        view_info.components.a = VK_COMPONENT_SWIZZLE_A;
        if (format == VK_FORMAT_D16_UNORM ||
            format == VK_FORMAT_D32_SFLOAT ||
            format == VK_FORMAT_D16_UNORM_S8_UINT ||
            format == VK_FORMAT_D24_UNORM_S8_UINT ||
            format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
            false)
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
        view_info.flags = 0;

        api_result = vkCreateImageView(device, &view_info, nullptr, &result.image_view);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Coudln't create view for image!");

        return result;
    }

} // namespace iceshard::renderer::vulkan
