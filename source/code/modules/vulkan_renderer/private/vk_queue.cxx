/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "vk_queue.hxx"
#include "vk_fence.hxx"

#include <ice/assert.hxx>

namespace ice::render::vk
{

    VulkanQueue::VulkanQueue(
        VkQueue vk_queue,
        VkDevice vk_device,
        ice::Array<VkCommandPool> vk_cmd_pools
    ) noexcept
        : _vk_queue{ vk_queue }
        , _vk_device{ vk_device }
        , _vk_cmd_pools{ ice::move(vk_cmd_pools) }
    {
        VkFenceCreateInfo fence_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        VkResult result = vkCreateFence(_vk_device, &fence_info, nullptr, &_vk_submit_fence);
        ICE_ASSERT(
            result == VK_SUCCESS,
            "Couldn't create fence!"
        );
    }

    VulkanQueue::~VulkanQueue() noexcept
    {
        vkDestroyFence(_vk_device, _vk_submit_fence, nullptr);
        for (VkCommandPool vk_pool : _vk_cmd_pools)
        {
            vkDestroyCommandPool(_vk_device, vk_pool, nullptr);
        }
    }

    void VulkanQueue::allocate_buffers(
        ice::u32 pool_index,
        ice::render::CommandBufferType type,
        ice::Span<ice::render::CommandBuffer> buffers
    ) noexcept
    {
        CommandBuffer* buffers_ptr = ice::span::begin(buffers);
        VkCommandBuffer* vk_buffers = reinterpret_cast<VkCommandBuffer*>(buffers_ptr);

        VkCommandBufferAllocateInfo alloc_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        alloc_info.level = type == CommandBufferType::Primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        alloc_info.commandPool = _vk_cmd_pools[pool_index];
        alloc_info.commandBufferCount = ice::count(buffers);

        VkResult result = vkAllocateCommandBuffers(
            _vk_device,
            &alloc_info,
            vk_buffers
        );

        ICE_ASSERT(
            result == VkResult::VK_SUCCESS,
            "Failed to allocate command buffers!"
        );
    }

    void VulkanQueue::reset_pool(
        ice::u32 pool_index
    ) noexcept
    {
        vkResetCommandPool(
            _vk_device,
            _vk_cmd_pools[pool_index],
            VkCommandPoolResetFlagBits::VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT
        );
    }

    void VulkanQueue::submit(
        ice::Span<ice::render::CommandBuffer const> buffers,
        ice::render::RenderFence const* fence
    ) noexcept
    {
        CommandBuffer const* buffers_ptr = ice::span::begin(buffers);
        VkCommandBuffer const* vk_buffers = reinterpret_cast<VkCommandBuffer const*>(buffers_ptr);

        VkSubmitInfo submit_info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = nullptr;
        submit_info.pWaitDstStageMask = nullptr;
        submit_info.commandBufferCount = ice::count(buffers);
        submit_info.pCommandBuffers = vk_buffers;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = nullptr;

        VkResult result = vkQueueSubmit(_vk_queue, 1, &submit_info, static_cast<VulkanFence const*>(fence)->native());
        ICE_ASSERT(
            result == VK_SUCCESS,
            "Couldn't submit command buffers to queue!"
        );
    }

    void VulkanQueue::present(ice::render::RenderSwapchain* swapchain) noexcept
    {
        VulkanSwapchain const* const vk_swapchain = static_cast<VulkanSwapchain*>(swapchain);

        ice::u32 indices = vk_swapchain->current_image_index();
        VkSwapchainKHR swapchains[1]{ vk_swapchain->handle() };

        VkPresentInfoKHR present_info{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        present_info.swapchainCount = 1;
        present_info.pSwapchains = swapchains;
        present_info.pImageIndices = &indices;
        present_info.pWaitSemaphores = NULL;
        present_info.waitSemaphoreCount = 0;
        present_info.pResults = NULL;

        VkResult result = vkQueuePresentKHR(_vk_queue, &present_info);
        ICE_ASSERT(
            result == VK_SUCCESS,
            "Failed to present framebuffer image!"
        );
    }

} // ice::render::vk
