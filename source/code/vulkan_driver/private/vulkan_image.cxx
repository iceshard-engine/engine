#include "vulkan_image.hxx"
#include <core/debug/assert.hxx>

#undef max

namespace render::vulkan
{

    VulkanImage::VulkanImage(VkDevice device_handle, VkImage image_handle, VkDeviceMemory memory_handle) noexcept
        : _device_handle{ device_handle }
        , _image_handle{ image_handle }
        , _memory_handle{ memory_handle }
    {
        VkImageViewCreateInfo view_info = {};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.pNext = nullptr;
        view_info.image = _image_handle;
        view_info.format = VK_FORMAT_D16_UNORM;
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

        auto api_result = vkCreateImageView(_device_handle, &view_info, nullptr, &_view_handle);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Coudln't create view for image!");
    }

    VulkanImage::~VulkanImage() noexcept
    {
        vkDestroyImageView(_device_handle, _view_handle, nullptr);
        vkDestroyImage(_device_handle, _image_handle, nullptr);
        vkFreeMemory(_device_handle, _memory_handle, nullptr);
    }

    auto create_depth_buffer_image(core::allocator& alloc, VulkanPhysicalDevice* physical_device) noexcept -> core::memory::unique_pointer<VulkanImage>
    {
        auto surface_capabilities = physical_device->surface_capabilities();
        auto device_memory_properties = physical_device->memory_properties();
        auto graphics_device = physical_device->graphics_device()->native_handle();

        VkImageCreateInfo image_info = {};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.pNext = nullptr;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.format = VK_FORMAT_D16_UNORM;
        image_info.extent.width = surface_capabilities.maxImageExtent.width;
        image_info.extent.height = surface_capabilities.maxImageExtent.height;
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

        VkImage depth_image_handle;
        auto api_result = vkCreateImage(graphics_device, &image_info, nullptr, &depth_image_handle);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create depth buffer image!");

        VkMemoryRequirements memory_requirements;
        vkGetImageMemoryRequirements(graphics_device, depth_image_handle, &memory_requirements);

        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.allocationSize = memory_requirements.size;
        alloc_info.memoryTypeIndex = std::numeric_limits<decltype(alloc_info.memoryTypeIndex)>::max();

        bool memory_type_found = physical_device->find_memory_type_index(memory_requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, alloc_info.memoryTypeIndex);
        IS_ASSERT(memory_type_found, "No memory type found for depth image.");

        VkDeviceMemory depth_image_memory;
        api_result = vkAllocateMemory(graphics_device, &alloc_info, nullptr, &depth_image_memory);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't allocate memory for depth image!");

        api_result = vkBindImageMemory(graphics_device, depth_image_handle, depth_image_memory, 0);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't bind memory to depth image!");

        return core::memory::make_unique<VulkanImage>(alloc, graphics_device, depth_image_handle, depth_image_memory);
    }

} // namespace render::vulkan
