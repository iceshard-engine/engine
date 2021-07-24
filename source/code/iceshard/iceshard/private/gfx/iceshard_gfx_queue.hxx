#pragma once
#include <ice/allocator.hxx>
#include <ice/pod/array.hxx>
#include <ice/engine_frame.hxx>
#include <ice/render/render_queue.hxx>
#include <ice/gfx/gfx_queue.hxx>
#include <ice/gfx/gfx_task.hxx>

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

        auto render_queue() noexcept -> ice::render::RenderQueue*;

        void reset() noexcept;

        void request_command_buffers(
            ice::render::CommandBufferType type,
            ice::Span<ice::render::CommandBuffer> buffers
        ) noexcept;

        void submit_command_buffers(
            ice::Span<ice::render::CommandBuffer> buffers,
            ice::render::RenderFence const* fence
        ) noexcept;

    private:
        ice::StringID const _name;
        ice::render::RenderCommands& _render_commands;
        ice::render::RenderQueue* _render_queue;
        ice::render::QueueFlags const _flags;
        ice::u32 _queue_pool_index;

        ice::u32 _cmd_buffers_used[2]{ };
        ice::pod::Array<ice::render::CommandBuffer> _primary;
        ice::pod::Array<ice::render::CommandBuffer> _secondary;
    };

} // namespace ice
