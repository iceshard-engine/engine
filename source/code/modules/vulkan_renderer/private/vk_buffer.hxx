/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include "vk_include.hxx"
#include "vk_memory_allocator.hxx"
#include <ice/render/render_buffer.hxx>

namespace ice::render::vk
{

    struct VulkanBuffer
    {
        VkBuffer vk_buffer;
        VmaAllocation vma_allocation;
    };

} // namespace ice::render::vk
