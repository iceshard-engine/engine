#pragma once
#include <ice/allocator.hxx>
#include <ice/pod/array.hxx>
#include <ice/engine_frame.hxx>
#include <ice/render/render_queue.hxx>
#include <ice/gfx/gfx_queue.hxx>

namespace ice::gfx
{

    class IceGfxQueue final : public ice::gfx::GfxQueue
    {
    public:
        IceGfxQueue(
            ice::Allocator& alloc,
            ice::render::RenderCommands& commands,
            ice::render::RenderQueue* queue,
            ice::u32 pool_index
        ) noexcept;
        ~IceGfxQueue() noexcept = default;

        bool presenting() const noexcept override;
        void set_presenting(bool is_presenting) noexcept override;

        auto render_queue() noexcept -> ice::render::RenderQueue*;

        void prepare() noexcept;

        void alloc_command_buffers(
            ice::render::CommandBufferType type,
            ice::Span<ice::render::CommandBuffer> buffers
        ) noexcept override;

        void submit_command_buffers(
            ice::Span<ice::render::CommandBuffer> buffers
        ) noexcept override;

        void execute_pass(
            ice::EngineFrame const& frame,
            ice::gfx::GfxPass* gfx_pass
        ) noexcept;

    private:
        ice::render::RenderCommands& _render_commands;
        ice::render::RenderQueue* _render_queue;
        ice::u32 _queue_pool_index;
        bool _presenting = false;

        ice::render::CommandBuffer _primary_commands[2];
    };

} // namespace ice
