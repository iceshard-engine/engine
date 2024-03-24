/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include "webgpu_utils.hxx"
#include "webgpu_commands.hxx"
#include "webgpu_command_buffer.hxx"

#include <ice/render/render_queue.hxx>
#include <ice/container/hashmap.hxx>

namespace ice::render::webgpu
{

    class WebGPUQueue : public ice::render::RenderQueue
    {
    public:
        WebGPUQueue(
            ice::Allocator& alloc,
            WGPUDevice wgpu_device,
            WGPUQueue wgpu_queue
        ) noexcept;
        ~WebGPUQueue() noexcept;

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
        WGPUDevice _wgpu_device;
        WGPUQueue _wgpu_queue;
        ice::HashMap<WebGPUCommandBuffer*> _wgpu_command_buffers;
    };

} // namespace ice::render::webgpu
