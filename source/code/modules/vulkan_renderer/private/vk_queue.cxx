/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "vk_queue.hxx"
#include "vk_fence.hxx"

#include <ice/assert.hxx>

namespace ice::render::vk
{

    VulkanQueue::VulkanQueue(
        ice::Allocator& alloc,
        VkQueue vk_queue,
        VkDevice vk_device,
        VkPhysicalDevice vk_physical_device,
        ice::Array<VkCommandPool> vk_cmd_pools,
        bool profiled
    ) noexcept
        : _allocator{ alloc }
        , _vk_queue{ vk_queue }
        , _vk_device{ vk_device }
        , _vk_physical_device{ vk_physical_device }
        , _vk_cmd_pools{ ice::move(vk_cmd_pools) }
        , _profiled{ profiled }
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
        VkCommandBuffer vk_temp_buffers[16];
        VkCommandBufferAllocateInfo alloc_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        alloc_info.level = type == CommandBufferType::Primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        alloc_info.commandPool = _vk_cmd_pools[pool_index];
        alloc_info.commandBufferCount = ice::count(buffers);
        ICE_ASSERT_CORE(alloc_info.commandBufferCount < 16);

        VkResult result = vkAllocateCommandBuffers(
            _vk_device,
            &alloc_info,
            vk_temp_buffers
        );

        ice::u32 idx = 0;
        for (ice::render::CommandBuffer& buffer : buffers)
        {
            VulkanCommandBuffer* buffer_obj = _allocator.create<VulkanCommandBuffer>();
            buffer_obj->buffer = vk_temp_buffers[idx];

#if IPT_ENABLED
            if (_profiled)
            {
                buffer_obj->tracy_ctx = TracyVkContextCalibrated(
                    _vk_physical_device,
                    _vk_device,
                    _vk_queue,
                    buffer_obj->buffer,
                    vk_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT,
                    vk_vkGetCalibratedTimestampsEXT
                );
            }
#endif
            buffer = VulkanCommandBuffer::handle(buffer_obj);
            idx += 1;
        }

        ICE_ASSERT(
            result == VkResult::VK_SUCCESS,
            "Failed to allocate command buffers!"
        );
    }

    void VulkanQueue::release_buffers(
        ice::u32 pool_index,
        ice::render::CommandBufferType type,
        ice::Span<ice::render::CommandBuffer> buffers
    ) noexcept
    {
        if (ice::span::empty(buffers))
        {
            return;
        }

        ice::u32 count = 0;
        VkCommandBuffer vk_temp_buffers[32];
        for (ice::render::CommandBuffer buffer : buffers)
        {
            VulkanCommandBuffer* buffer_obj = VulkanCommandBuffer::native(buffer);
            vk_temp_buffers[count] = buffer_obj->buffer;
#if IPT_ENABLED
            if (_profiled)
            {
                TracyVkDestroy(buffer_obj->tracy_ctx);
            }
#endif
            _allocator.destroy(buffer_obj);
            count += 1;
        }

        ICE_ASSERT_CORE(count < 32);
        vkFreeCommandBuffers(_vk_device, _vk_cmd_pools[pool_index], count, vk_temp_buffers);
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
        ice::render::RenderFence* fence
    ) noexcept
    {
        VkCommandBuffer vk_temp_buffers[16];
        ICE_ASSERT_CORE(ice::count(buffers) < 16);

        ice::u32 count = 0;
        for (ice::render::CommandBuffer handle : buffers)
        {
            vk_temp_buffers[count] = VulkanCommandBuffer::native(handle)->buffer;
            count += 1;
        }

        VkSubmitInfo submit_info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = nullptr;
        submit_info.pWaitDstStageMask = nullptr;
        submit_info.commandBufferCount = count;
        submit_info.pCommandBuffers = vk_temp_buffers;
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
            result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR,
            "Failed to present framebuffer image!"
        );
    }

} // ice::render::vk
