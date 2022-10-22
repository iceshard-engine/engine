/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include "vk_include.hxx"
#include "vk_memory_manager.hxx"
#include <ice/render/render_buffer.hxx>

namespace ice::render::vk
{

    struct VulkanBuffer
    {
        VkBuffer vk_buffer;
        AllocationHandle vk_alloc_handle;
    };

} // namespace ice::render::vk
