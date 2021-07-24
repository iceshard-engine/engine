#include "iceshard_gfx_queue.hxx"

#include <ice/gfx/gfx_stage.hxx>
#include <ice/gfx/gfx_pass.hxx>
#include <ice/memory/stack_allocator.hxx>

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
        ice::pod::array::resize(_primary, 1);
        _render_queue->allocate_buffers(
            _queue_pool_index,
            ice::render::CommandBufferType::Primary,
            _primary
        );
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
        ice::Span<ice::render::CommandBuffer> buffers
    ) noexcept
    {
        using ice::render::CommandBuffer;
        using ice::render::CommandBufferType;

        ice::pod::Array<CommandBuffer>& cmds = (type == CommandBufferType::Primary) ? _primary : _secondary;
        ice::u32& used = _cmd_buffers_used[static_cast<ice::u32>(type)];
        ice::u32 const available = ice::pod::array::size(cmds) - used;
        ice::u32 const required = ice::size(buffers);

        if (available < required)
        {
            ice::pod::array::resize(cmds, used + required);
            _render_queue->allocate_buffers(_queue_pool_index, type, ice::pod::array::span(cmds, used));
        }

        auto from = ice::pod::array::span(cmds, used);

        for (ice::u32 idx = 0; idx < required; ++idx)
        {
            buffers[idx] = from[idx];
        }
        used += required;
    }

    void IceGfxQueue::submit_command_buffers(
        ice::Span<ice::render::CommandBuffer> buffers,
        ice::render::RenderFence const* fence
    ) noexcept
    {
        _render_queue->submit(buffers, fence);
    }

} // namespace ice::gfx
