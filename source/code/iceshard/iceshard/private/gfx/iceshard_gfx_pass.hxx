#pragma once
#include <ice/allocator.hxx>
#include <ice/pod/array.hxx>
#include <ice/render/render_queue.hxx>
#include <ice/gfx/gfx_pass.hxx>

namespace ice::gfx
{

    class IceGfxPass final : public ice::gfx::GfxPass
    {
    public:
        IceGfxPass(
            ice::Allocator& alloc,
            ice::render::RenderCommands& commands,
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

        void add_stage(
            ice::StringID_Arg name,
            ice::gfx::GfxStage* stage,
            ice::Span<ice::gfx::GfxStage*> fence_wait
        ) noexcept override;

        void execute() noexcept;

    private:
        ice::render::RenderCommands& _render_commands;
        ice::render::RenderQueue* _render_queue;
        ice::u32 _queue_pool_index;
        bool _presenting = false;

        ice::render::CommandBuffer _primary_commands;
        ice::pod::Array<ice::gfx::GfxStage*> _stages;
    };

} // namespace ice
