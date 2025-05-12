/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "webgpu_queue.hxx"
#include "webgpu_fence.hxx"
#include "webgpu_command_buffer.hxx"
#include "webgpu_swapchain.hxx"
#include <emscripten/html5.h>

namespace ice::render::webgpu
{

    WebGPUQueue::WebGPUQueue(
        ice::Allocator& alloc,
        WGPUDevice wgpu_device,
        WGPUQueue wgpu_queue
    ) noexcept
        : _allocator{ alloc }
        , _wgpu_device{ wgpu_device }
        , _wgpu_queue{ wgpu_queue }
        , _wgpu_command_buffers{ _allocator }
    {
    }

    WebGPUQueue::~WebGPUQueue() noexcept
    {
        for (WebGPUCommandBuffer* wgpu_cmds : _wgpu_command_buffers)
        {
            if (wgpu_cmds->command_buffer != nullptr)
            {
                wgpuCommandBufferRelease(ice::exchange(wgpu_cmds->command_buffer, nullptr));
            }

            wgpuCommandEncoderRelease(wgpu_cmds->command_encoder);
        }
        wgpuQueueRelease(_wgpu_queue);
    }

    void WebGPUQueue::allocate_buffers(
        ice::u32 pool_index,
        ice::render::CommandBufferType type,
        ice::Span<ice::render::CommandBuffer> buffers
    ) noexcept
    {
        for (ice::render::CommandBuffer& out_buffer : buffers)
        {
            WebGPUCommandBuffer* webgpu_cmds = _allocator.create<WebGPUCommandBuffer>();
            webgpu_cmds->command_buffer = nullptr;
            webgpu_cmds->type = type;

            WGPUCommandEncoderDescriptor descriptor{};
            descriptor.label = type == CommandBufferType::Primary ? "Primary Command Encoded" : "Secondary Command Encoder";
            webgpu_cmds->command_encoder = wgpuDeviceCreateCommandEncoder(_wgpu_device, &descriptor);

            out_buffer = WebGPUCommandBuffer::handle(webgpu_cmds);
            ice::multi_hashmap::insert(_wgpu_command_buffers, pool_index, webgpu_cmds);
        }
    }

    void WebGPUQueue::release_buffers(
        ice::u32 pool_index,
        ice::render::CommandBufferType type,
        ice::Span<ice::render::CommandBuffer> buffers
    ) noexcept
    {
        for (ice::render::CommandBuffer buffer : buffers)
        {
            WebGPUCommandBuffer* webgpu_cmds = WebGPUCommandBuffer::native(buffer);
            if (webgpu_cmds->command_buffer != nullptr)
            {
                wgpuCommandBufferRelease(webgpu_cmds->command_buffer);
            }
            wgpuCommandEncoderRelease(webgpu_cmds->command_encoder);

            WGPUCommandEncoderDescriptor descriptor{};
            descriptor.label = type == CommandBufferType::Primary ? "Primary Command Encoded" : "Secondary Command Encoder";
            webgpu_cmds->command_encoder = wgpuDeviceCreateCommandEncoder(_wgpu_device, &descriptor);

            auto it = ice::multi_hashmap::find_first(_wgpu_command_buffers, pool_index);
            while (it != nullptr)
            {
                auto next = ice::multi_hashmap::find_next(_wgpu_command_buffers, it);
                if (it.value() == webgpu_cmds)
                {
                    ice::multi_hashmap::remove(_wgpu_command_buffers, it);
                    _allocator.destroy(webgpu_cmds);
                }
                it = next;
            }
        }
    }

    void WebGPUQueue::reset_pool(
        ice::u32 pool_index
    ) noexcept
    {
        auto it = ice::multi_hashmap::find_first(_wgpu_command_buffers, pool_index);
        while (it != nullptr)
        {
            // if (it.value()->type == CommandBufferType::Secondary)
            // {
            //     if (it.value()->command_buffer != nullptr)
            //     {
            //         wgpuCommandBufferRelease(ice::exchange(it.value()->command_buffer, nullptr));
            //     }

            //     wgpuCommandEncoderRelease(it.value()->command_encoder);
            //     auto next = ice::multi_hashmap::find_next(_wgpu_command_buffers, it);
            //     ice::multi_hashmap::remove(_wgpu_command_buffers, it);
            //     it = next;
            // }
            // else
            {
                if (it.value()->command_buffer != nullptr)
                {
                    wgpuCommandBufferRelease(ice::exchange(it.value()->command_buffer, nullptr));
                }

                wgpuCommandEncoderRelease(it.value()->command_encoder);
                WGPUCommandEncoderDescriptor descriptor{};
                if (it.value()->type == CommandBufferType::Secondary)
                {
                    descriptor.label = "Primary Command Encoded";
                }
                else
                {
                    descriptor.label = "Secondary Command Encoded";
                }
                it.value()->command_encoder = wgpuDeviceCreateCommandEncoder(_wgpu_device, &descriptor);

                it = ice::multi_hashmap::find_next(_wgpu_command_buffers, it);
            }
        }

    }

    void WebGPUQueue::submit(
        ice::Span<ice::render::CommandBuffer const> buffers,
        ice::render::RenderFence* fence
    ) noexcept
    {
        WebGPUCallbackFence* wgpu_fence = static_cast<WebGPUCallbackFence*>(fence);

        ice::ucount wgpu_cb_count = 0;
        WGPUCommandBuffer wgpu_cb_list[16];

        for (ice::render::CommandBuffer cmdbuff : buffers)
        {
            WebGPUCommandBuffer* wgpu_cmds = WebGPUCommandBuffer::native(cmdbuff);
            ICE_ASSERT_CORE(wgpu_cmds->command_buffer != nullptr); // Ensure the buffer was finalized.

            // Aquire the finalized command buffer
            wgpu_cb_list[wgpu_cb_count] = wgpu_cmds->command_buffer;
            wgpu_cb_count += 1;
        }

        //wgpuQueueOnSubmittedWorkDone(_wgpu_queue, WebGPUCallbackFence::wgpu_on_queue_done_callback, wgpu_fence);
        wgpuQueueSubmit(_wgpu_queue, wgpu_cb_count, wgpu_cb_list);

        // On WebGPU (WebBrowsers) we can set the fence immediately to done due to how the app life-cycle works on browsers.
        wgpu_fence->set();
    }

    void WebGPUQueue::present(
        ice::render::RenderSwapchain* swapchain
    ) noexcept
    {
        // On WebApp's we don't explicitly present anything, this is done by the browser
    }

} // namespace ice::render::webgpu
