#include "vulkan_image.hxx"
#include <core/debug/assert.hxx>

#undef max

namespace render::vulkan
{

    VulkanImage::VulkanImage(
        VkDevice const device_handle,
        VkImage const image,
        VkImageView const image_view,
        VulkanMemoryInfo const memory_info
    ) noexcept
        : _device_handle{ device_handle }
        , _image{ image }
        , _image_view{ image_view }
        , _image_memory{ std::move(memory_info) }
    {
    }

    VulkanImage::~VulkanImage() noexcept
    {
        vkDestroyImageView(_device_handle, _image_view, nullptr);
        vkDestroyImage(_device_handle, _image, nullptr);
    }

    auto create_depth_buffer_image(
        core::allocator& alloc,
        VulkanDeviceMemoryManager& device_memory,
        VkExtent2D extent
    ) noexcept -> core::memory::unique_pointer<VulkanImage>
    {
        auto graphics_device = device_memory.graphics_device();

        VkImageCreateInfo image_info = {};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.pNext = nullptr;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.format = VK_FORMAT_D16_UNORM;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.extent.width = extent.width;
        image_info.extent.height = extent.height;
        image_info.extent.depth = 1;
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        image_info.queueFamilyIndexCount = 0;
        image_info.pQueueFamilyIndices = nullptr;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.flags = 0;

        VkImage image;
        auto api_result = vkCreateImage(graphics_device, &image_info, nullptr, &image);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create depth buffer image!");

        VulkanMemoryInfo memory_info;
        device_memory.allocate_memory(
            image,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            memory_info
        );

        VkImageViewCreateInfo view_info = {};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.pNext = nullptr;
        view_info.image = image;
        view_info.format = image_info.format;
        view_info.components.r = VK_COMPONENT_SWIZZLE_R;
        view_info.components.g = VK_COMPONENT_SWIZZLE_G;
        view_info.components.b = VK_COMPONENT_SWIZZLE_B;
        view_info.components.a = VK_COMPONENT_SWIZZLE_A;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.flags = 0;

        VkImageView image_view;
        api_result = vkCreateImageView(graphics_device, &view_info, nullptr, &image_view);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Coudln't create view for image!");

        return core::memory::make_unique<VulkanImage>(alloc, graphics_device, image, image_view, std::move(memory_info));
    }

    auto create_texture_2d(
        core::allocator& alloc,
        VulkanDeviceMemoryManager& device_memory,
        VkExtent2D extent
    ) noexcept -> core::memory::unique_pointer<VulkanImage>
    {
        auto graphics_device = device_memory.graphics_device();

        VkImageCreateInfo image_info = {};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.pNext = nullptr;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.extent.width = extent.width;
        image_info.extent.height = extent.height;
        image_info.extent.depth = 1;
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image_info.queueFamilyIndexCount = 0;
        image_info.pQueueFamilyIndices = nullptr;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.flags = 0;

        VkImage image;
        auto api_result = vkCreateImage(graphics_device, &image_info, nullptr, &image);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create depth buffer image!");

        VulkanMemoryInfo memory_info;
        device_memory.allocate_memory(
            image,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            memory_info
        );

        return core::memory::make_unique<VulkanImage>(alloc, graphics_device, image, nullptr, std::move(memory_info));
    }

} // namespace render::vulkan
