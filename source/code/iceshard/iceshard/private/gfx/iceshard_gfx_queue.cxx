#include "iceshard_gfx_queue.hxx"
#include "iceshard_gfx_pass.hxx"
#include <ice/gfx/gfx_stage.hxx>

namespace ice::gfx
{

    IceGfxQueue::IceGfxQueue(
        ice::Allocator& alloc,
        ice::render::RenderCommands& commands,
        ice::render::RenderQueue* queue,
        ice::u32 pool_index
    ) noexcept
        : ice::gfx::GfxQueue{ }
        , _render_commands{ commands }
        , _render_queue{ queue }
        , _queue_pool_index{ pool_index }
    {
    }

    bool IceGfxQueue::presenting() const noexcept
    {
        return _presenting;
    }

    void IceGfxQueue::set_presenting(bool is_presenting) noexcept
    {
        _presenting = is_presenting;
    }

    auto IceGfxQueue::render_queue() noexcept -> ice::render::RenderQueue*
    {
        return _render_queue;
    }

    void IceGfxQueue::prepare() noexcept
    {
        _render_queue->reset_pool(_queue_pool_index);
        _render_queue->allocate_buffers(
            _queue_pool_index,
            ice::render::CommandBufferType::Primary,
            ice::Span<ice::render::CommandBuffer>{ &_primary_commands, 1 }
        );
    }

    void IceGfxQueue::alloc_command_buffers(
        ice::render::CommandBufferType type,
        ice::Span<ice::render::CommandBuffer> buffers
    ) noexcept
    {
        _render_queue->allocate_buffers(_queue_pool_index, type, buffers);
    }

    void IceGfxQueue::execute_pass(ice::gfx::GfxPass* gfx_pass) noexcept
    {
        ice::gfx::IceGfxPass* const ice_pass = static_cast<ice::gfx::IceGfxPass*>(gfx_pass);
        if (ice_pass->has_work())
        {
            ice_pass->record_commands(_primary_commands, _render_commands);
        }

        _render_queue->submit(
            ice::Span<ice::render::CommandBuffer>{ &_primary_commands, 1 }
        );
    }

} // namespace ice::gfx
