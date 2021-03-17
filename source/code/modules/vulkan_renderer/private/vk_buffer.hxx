#pragma once
#include "vk_include.hxx"
#include "vk_memory_manager.hxx"

namespace ice::render::vk
{

    struct VulkanBuffer
    {
        VkBuffer vk_buffer;
        AllocationHandle vk_alloc_handle;
    };

} // namespace ice::render::vk
