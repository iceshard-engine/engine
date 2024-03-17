/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/container/array.hxx>
#include <ice/engine_frame.hxx>
#include <ice/render/render_queue.hxx>
#include <ice/gfx/gfx_queue.hxx>

namespace ice::gfx
{

    struct GfxStageSlot;

    class IceGfxQueue final : public ice::gfx::GfxQueue
    {
    public:
        IceGfxQueue(
            ice::Allocator& alloc,
            ice::StringID_Arg name,
            ice::render::RenderCommands& commands,
            ice::render::RenderQueue* queue,
            ice::render::QueueFlags const flags,
            ice::u32 pool_index
        ) noexcept;
        ~IceGfxQueue() noexcept = default;

        auto name() const noexcept -> ice::StringID_Arg;
        auto queue_flags() const noexcept -> ice::render::QueueFlags;

        bool presenting() const noexcept override;

        void present(ice::render::RenderSwapchain* swapchain) const noexcept override;

        auto render_queue() noexcept -> ice::render::RenderQueue*;

        void reset() noexcept override;

        void request_command_buffers(
            ice::render::CommandBufferType type,
            ice::Span<ice::render::CommandBuffer> out_buffers
        ) noexcept override;

        void submit_command_buffers(
            ice::Span<ice::render::CommandBuffer const> buffers,
            ice::render::RenderFence* fence
        ) noexcept override;

    private:
        ice::StringID const _name;
        ice::render::RenderCommands& _render_commands;
        ice::render::RenderQueue* _render_queue;
        ice::render::QueueFlags const _flags;
        ice::u32 _queue_pool_index;

        ice::u32 _cmd_buffers_used[2]{ };
        ice::Array<ice::render::CommandBuffer> _primary;
        ice::Array<ice::render::CommandBuffer> _secondary;
    };

} // namespace ice
