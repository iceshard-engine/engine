/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/render/render_queue.hxx>
#include <ice/container/array.hxx>
#include "vk_swapchain.hxx"
#include "vk_include.hxx"

namespace ice::render::vk
{

    class VulkanQueue final : public ice::render::RenderQueue
    {
    public:
        VulkanQueue(
            VkQueue vk_queue,
            VkDevice vk_device,
            ice::Array<VkCommandPool> vk_cmd_pools
        ) noexcept;
        ~VulkanQueue() noexcept override;

        void allocate_buffers(
            ice::u32 pool_index,
            ice::render::CommandBufferType type,
            ice::Span<ice::render::CommandBuffer> buffers
        ) noexcept override;

        void reset_pool(
            ice::u32 pool_index
        ) noexcept override;

        void submit(
            ice::Span<ice::render::CommandBuffer const> buffers,
            ice::render::RenderFence const* fence
        ) noexcept override;

        void present(
            ice::render::RenderSwapchain* swapchain
        ) noexcept override;

    private:
        VkQueue _vk_queue;
        VkDevice _vk_device;
        VkFence _vk_submit_fence;

        ice::Array<VkCommandPool> _vk_cmd_pools;
    };

} // namespace ice::render::vk
