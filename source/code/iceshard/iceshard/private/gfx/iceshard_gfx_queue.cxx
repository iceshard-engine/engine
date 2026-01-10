/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_gfx_queue.hxx"

#include <ice/gfx/gfx_stage.hxx>
#include <ice/mem_allocator_stack.hxx>

namespace ice::gfx
{

    using ice::render::QueueFlags;

    IceGfxQueue::IceGfxQueue(
        ice::Allocator& alloc,
        ice::StringID_Arg name,
        ice::render::RenderCommands& commands,
        ice::render::RenderQueue* queue,
        ice::render::QueueFlags const flags,
        ice::u32 pool_index
    ) noexcept
        : ice::gfx::GfxQueue{ }
        , _name{ name }
        , _render_commands{ commands }
        , _render_queue{ queue }
        , _flags{ flags }
        , _queue_pool_index{ pool_index }
        , _primary{ alloc }
        , _secondary{ alloc }
    {
        ice::array::resize(_primary, 1);
        _render_queue->allocate_buffers(
            _queue_pool_index,
            ice::render::CommandBufferType::Primary,
            _primary
        );
    }

    IceGfxQueue::~IceGfxQueue() noexcept
    {
        _render_queue->release_buffers(_queue_pool_index, ice::render::CommandBufferType::Primary, _primary);
        _render_queue->release_buffers(_queue_pool_index, ice::render::CommandBufferType::Secondary, _secondary);
    }

    auto IceGfxQueue::name() const noexcept -> ice::StringID_Arg
    {
        return _name;
    }

    auto IceGfxQueue::queue_flags() const noexcept -> ice::render::QueueFlags
    {
        return _flags;
    }

    bool IceGfxQueue::presenting() const noexcept
    {
        return (_flags & QueueFlags::Present) == QueueFlags::Present;
    }

    void IceGfxQueue::present(ice::render::RenderSwapchain* swapchain) const noexcept
    {
        _render_queue->present(swapchain);
    }

    auto IceGfxQueue::render_queue() noexcept -> ice::render::RenderQueue*
    {
        return _render_queue;
    }

    void IceGfxQueue::reset() noexcept
    {
        _render_queue->reset_pool(_queue_pool_index);
        _cmd_buffers_used[0] = 0;
        _cmd_buffers_used[1] = 0;
    }

    void IceGfxQueue::request_command_buffers(
        ice::render::CommandBufferType type,
        ice::Span<ice::render::CommandBuffer> out_buffers
    ) noexcept
    {
        using ice::render::CommandBuffer;
        using ice::render::CommandBufferType;

        ice::Array<CommandBuffer>& cmds = (type == CommandBufferType::Primary) ? _primary : _secondary;
        ice::u32& used = _cmd_buffers_used[static_cast<ice::u32>(type)];
        ice::u32 const available = cmds.size().u32() - used;
        ice::u32 const required = out_buffers.size().u32();

        if (available < required)
        {
            cmds.resize(used + required);
            _render_queue->allocate_buffers(_queue_pool_index, type, ice::array::slice(cmds, used));
        }

        auto from = ice::array::slice(cmds, used);

        for (ice::u32 idx = 0; idx < required; ++idx)
        {
            out_buffers[idx] = from[idx];
        }
        used += required;
    }

    void IceGfxQueue::submit_command_buffers(
        ice::Span<ice::render::CommandBuffer const> buffers,
        ice::render::RenderFence* fence
    ) noexcept
    {
        _render_queue->submit(buffers, fence);
    }

} // namespace ice::gfx
