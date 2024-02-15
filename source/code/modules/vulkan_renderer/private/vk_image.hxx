/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include "vk_include.hxx"
#include "vk_memory_allocator.hxx"

namespace ice::render::vk
{

    struct VulkanImage
    {
        VkImage vk_image;
        VkImageView vk_image_view;
        VmaAllocation vma_allocation;
    };

} // namespace ice::render::vk
