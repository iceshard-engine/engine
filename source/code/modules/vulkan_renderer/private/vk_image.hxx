#pragma once
#include "vk_include.hxx"
#include "vk_memory_manager.hxx"

namespace ice::render::vk
{

    struct VulkanImage
    {
        VkImage vk_image;
        VkImageView vk_image_view;
        AllocationHandle vk_alloc_handle;
    };

} // namespace ice::render::vk
