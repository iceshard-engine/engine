/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/render/render_queue.hxx>
#include <ice/array.hxx>
#include "vk_command_buffer.hxx"
#include "vk_swapchain.hxx"
#include "vk_include.hxx"

namespace ice::render::vk
{

    class VulkanQueue final : public ice::render::RenderQueue
    {
    public:
        VulkanQueue(
            ice::Allocator& alloc,
            VkQueue vk_queue,
            VkDevice vk_device,
            VkPhysicalDevice vk_physical_device,
            ice::Array<VkCommandPool> vk_cmd_pools,
            bool profiled = false
        ) noexcept;
        ~VulkanQueue() noexcept override;

        void allocate_buffers(
            ice::u32 pool_index,
            ice::render::CommandBufferType type,
            ice::Span<ice::render::CommandBuffer> buffers
        ) noexcept override;

        void release_buffers(
            ice::u32 pool_index,
            ice::render::CommandBufferType type,
            ice::Span<ice::render::CommandBuffer> buffers
        ) noexcept override;

        void reset_pool(
            ice::u32 pool_index
        ) noexcept override;

        void submit(
            ice::Span<ice::render::CommandBuffer const> buffers,
            ice::render::RenderFence* fence
        ) noexcept override;

        void present(
            ice::render::RenderSwapchain* swapchain
        ) noexcept override;

    private:
        ice::Allocator& _allocator;
        VkQueue _vk_queue;
        VkDevice _vk_device;
        VkPhysicalDevice _vk_physical_device;
        VkFence _vk_submit_fence;

        ice::Array<VkCommandPool> _vk_cmd_pools;
        bool const _profiled;
    };

} // namespace ice::render::vk
