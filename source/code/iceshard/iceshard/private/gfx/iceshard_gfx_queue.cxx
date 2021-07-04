#include "iceshard_gfx_queue.hxx"

#include <ice/gfx/gfx_stage.hxx>
#include <ice/gfx/gfx_pass.hxx>
#include <ice/memory/stack_allocator.hxx>

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
        _render_queue->allocate_buffers(
            _queue_pool_index,
            ice::render::CommandBufferType::Primary,
            _primary_commands
        );
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
    }

    void IceGfxQueue::alloc_command_buffers(
        ice::render::CommandBufferType type,
        ice::Span<ice::render::CommandBuffer> buffers
    ) noexcept
    {
        _render_queue->allocate_buffers(_queue_pool_index, type, buffers);
    }

    void IceGfxQueue::submit_command_buffers(
        ice::Span<ice::render::CommandBuffer> buffers
    ) noexcept
    {
        _render_queue->submit(buffers, false);
    }

    void IceGfxQueue::test_begin() noexcept
    {
        _render_commands.begin(_primary_commands[0]);
    }

    void IceGfxQueue::test_end() noexcept
    {
        _render_commands.end(_primary_commands[0]);
        _render_queue->submit(
            ice::Span<ice::render::CommandBuffer>{ _primary_commands + 0, 1 }, true
        );
    }

    void IceGfxQueue::execute_pass(
        ice::EngineFrame const& frame,
        ice::gfx::GfxPass const* gfx_pass,
        ice::pod::Hash<ice::gfx::GfxStageSlot> const& stages
    ) noexcept
    {
        ice::memory::StackAllocator_1024 alloc;

        ice::pod::Array<ice::StringID_Hash> stage_order{ alloc };
        gfx_pass->query_stage_order(stage_order);

        bool has_all = true;
        for (ice::StringID_Hash stage_id : stage_order)
        {
            has_all &= ice::pod::hash::has(stages, ice::hash(stage_id));
        }

        if (has_all)
        {
            for (ice::StringID_Hash stage_id : stage_order)
            {
                ice::gfx::GfxStage const* stage = ice::pod::hash::get(
                    stages,
                    ice::hash(stage_id),
                    ice::gfx::GfxStageSlot{ }
                ).stage;

                stage->record_commands(frame, _primary_commands[1], _render_commands);
            }

            _render_queue->submit(
                ice::Span<ice::render::CommandBuffer>{ _primary_commands + 1, 1 }, true
            );
        }
    }

    void IceGfxQueue::update_texture(
        ice::render::Image image,
        ice::render::Buffer image_contents,
        ice::vec2u extents
    ) noexcept
    {
        _render_commands.update_texture(_primary_commands[0], image, image_contents, extents);
    }

} // namespace ice::gfx
