/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include "vk_fence.hxx"
#include <ice/assert.hxx>

namespace ice::render::vk
{

    VulkanFence::VulkanFence(VkDevice vk_device, VkFence vk_fence) noexcept
        : _vk_device{ vk_device }
        , _vk_fence{ vk_fence }
    {
    }

    VulkanFence::~VulkanFence() noexcept
    {
        vkDestroyFence(_vk_device, _vk_fence, nullptr);
    }

    auto VulkanFence::native() const noexcept -> VkFence
    {
        return _vk_fence;
    }

    bool VulkanFence::wait(ice::u64 timeout_ns) noexcept
    {
        VkResult const result = vkWaitForFences(_vk_device, 1, &_vk_fence, true, timeout_ns);
        ICE_ASSERT(
            result == VK_SUCCESS || result == VK_TIMEOUT,
            "Device lost during fence wait!"
        );
        return result != VK_TIMEOUT;
    }

    void VulkanFence::reset() noexcept
    {
        vkResetFences(_vk_device, 1, &_vk_fence);
    }

} // namespace ice::render::vk
