#pragma once
#include <ice/gfx/gfx_frame.hxx>
#include <ice/gfx/gfx_queue.hxx>
#include <ice/gfx/gfx_context.hxx>

#include <ice/render/render_device.hxx>
#include <ice/engine_frame.hxx>


#include <ice/memory/proxy_allocator.hxx>
#include <ice/collections.hxx>
#include <ice/task.hxx>
#include <ice/task_list.hxx>

#include "../iceshard_task_executor.hxx"
#include "iceshard_gfx_queue_group.hxx"

namespace ice::gfx
{

    struct IceGfxPassEntry
    {
        ice::StringID queue_name;
        ice::StringID pass_name;
        ice::gfx::GfxPass const* pass;
    };

    class IceGfxFrame final : public ice::gfx::GfxFrame
    {
    public:
        IceGfxFrame(
            ice::Allocator& alloc
        ) noexcept;

        ~IceGfxFrame() noexcept override = default;

        void add_task(ice::Task<> task) noexcept override;

        void set_stage_slot(
            ice::StringID_Arg stage_name,
            ice::gfx::GfxContextStage const* stage
        ) noexcept override;

        void enqueue_pass(
            ice::StringID_Arg queue_name,
            ice::StringID_Arg pass_name,
            ice::gfx::GfxPass const* pass
        ) noexcept override;

        bool query_queue_stages(
            ice::Span<ice::StringID_Hash const> stage_order,
            ice::Span<ice::gfx::GfxContextStage const*> out_stages
        ) noexcept;

        auto enqueued_passes() const noexcept -> ice::Span<ice::gfx::IceGfxPassEntry const>;

        auto create_task_executor() noexcept -> ice::IceshardTaskExecutor;

        void on_frame_begin() noexcept;
        bool on_frame_stage(
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer command_buffer,
            ice::render::RenderCommands& render_commands
        ) noexcept;
        void on_frame_end() noexcept;

    protected:
        void schedule_internal(
            ice::gfx::GfxAwaitBeginFrameData& operation
        ) noexcept override;

        void schedule_internal(
            ice::gfx::GfxAwaitExecuteStageData& operation
        ) noexcept override;

        void schedule_internal(
            ice::gfx::GfxAwaitEndFrameData& operation
        ) noexcept override;

    private:
        ice::Allocator& _allocator;
        ice::Vector<ice::Task<>> _tasks;
        ice::pod::Array<ice::gfx::IceGfxPassEntry> _passes;

        ice::pod::Hash<ice::gfx::GfxFrameStage const*> _frame_stages;
        ice::pod::Hash<ice::gfx::GfxContextStage const*> _context_stages;

        std::atomic<ice::gfx::GfxAwaitBeginFrameData*> _operations_frame_begin = nullptr;
        std::atomic<ice::gfx::GfxAwaitExecuteStageData*> _operations_stage_execute = nullptr;
        std::atomic<ice::gfx::GfxAwaitEndFrameData*> _operations_frame_end = nullptr;
    };

} // namespace ice::gfx
