#include "iceshard_gfx_pass.hxx"

namespace ice::gfx
{

    IceGfxPass::IceGfxPass(
        ice::render::RenderQueue* queue,
        ice::u32 pool_index
    ) noexcept
        : ice::gfx::GfxPass{ }
        , _render_queue{ queue }
        , _queue_pool_index{ pool_index }
    {
    }

    bool IceGfxPass::presenting() const noexcept
    {
        return _presenting;
    }

    void IceGfxPass::set_presenting(bool is_presenting) noexcept
    {
        _presenting = is_presenting;
    }

    auto IceGfxPass::render_queue() noexcept -> ice::render::RenderQueue*
    {
        return _render_queue;
    }

    void IceGfxPass::prepare() noexcept
    {
        _render_queue->reset_pool(_queue_pool_index);
    }

    void IceGfxPass::alloc_command_buffers(
        ice::render::CommandBufferType type,
        ice::Span<ice::render::CommandBuffer> buffers
    ) noexcept
    {
        _render_queue->allocate_buffers(_queue_pool_index, type, buffers);
    }

    auto IceGfxPass::add_stage(
        ice::StringID_Arg name,
        ice::Span<ice::gfx::GfxStage*> fence_wait
    ) noexcept -> ice::gfx::GfxStage*
    {
        return nullptr;
    }

    void IceGfxPass::execute() noexcept
    {
    }

} // namespace ice::gfx
