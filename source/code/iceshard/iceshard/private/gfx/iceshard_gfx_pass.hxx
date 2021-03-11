#pragma once
#include <ice/gfx/gfx_pass.hxx>
#include <ice/render/render_queue.hxx>

namespace ice::gfx
{

    class IceGfxPass final : public ice::gfx::GfxPass
    {
    public:
        IceGfxPass(
            ice::render::RenderQueue* queue,
            ice::u32 pool_index
        ) noexcept;
        ~IceGfxPass() noexcept = default;

        bool presenting() const noexcept override;
        void set_presenting(bool is_presenting) noexcept override;

        auto render_queue() noexcept -> ice::render::RenderQueue*;

        void prepare() noexcept;

        void alloc_command_buffers(
            ice::render::CommandBufferType type,
            ice::Span<ice::render::CommandBuffer> buffers
        ) noexcept;

        auto add_stage(
            ice::StringID_Arg name,
            ice::Span<ice::gfx::GfxStage*> fence_wait
        ) noexcept -> ice::gfx::GfxStage* override;

        void execute() noexcept;

    private:
        ice::render::RenderQueue* _render_queue;
        ice::u32 _queue_pool_index;
        bool _presenting = false;
    };

} // namespace ice
