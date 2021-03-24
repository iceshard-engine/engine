#include "iceshard_gfx_queue.hxx"
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
        , _stages{ alloc }
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
        ice::pod::array::clear(_stages);

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

    void IceGfxQueue::add_stage(
        ice::StringID_Arg name,
        ice::gfx::GfxStage* stage,
        ice::Span<ice::gfx::GfxStage*> fence_wait
    ) noexcept
    {
        ice::pod::array::push_back(
            _stages,
            stage
        );
    }

    void IceGfxQueue::execute() noexcept
    {
        bool const contains_work = ice::pod::array::empty(_stages) == false;
        if (contains_work)
        {
            for (ice::gfx::GfxStage* stage : _stages)
            {
                stage->record_commands(
                    _primary_commands,
                    _render_commands
                );
            }

            _render_queue->submit(
                ice::Span<ice::render::CommandBuffer>{ &_primary_commands, 1 }
            );
        }
    }

} // namespace ice::gfx
