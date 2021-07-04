#pragma once
#include <ice/gfx/gfx_frame.hxx>
#include <ice/gfx/gfx_queue.hxx>
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

    class IceGfxTaskFrame : public ice::gfx::GfxFrame
    {
    public:
        IceGfxTaskFrame(ice::Allocator& alloc) noexcept;
        ~IceGfxTaskFrame() noexcept override = default;

        void execute_task(ice::Task<> task) noexcept;

        auto frame_start() noexcept -> ice::gfx::GfxFrameStartOperation override;
        auto frame_commands(ice::StringID_Arg queue_name) noexcept -> ice::gfx::GfxFrameCommandsOperation override;
        auto frame_end() noexcept -> ice::gfx::GfxFrameEndOperation override;

        void schedule_internal(
            ice::gfx::GfxFrameStartOperation* operation,
            ice::gfx::GfxFrameStartOperation::DataMemberType data
        ) noexcept override;

        void schedule_internal(
            ice::gfx::GfxFrameCommandsOperation* operation,
            ice::gfx::GfxFrameCommandsOperation::DataMemberType data
        ) noexcept override;

        void schedule_internal(
            ice::gfx::GfxFrameEndOperation* operation,
            ice::gfx::GfxFrameEndOperation::DataMemberType data
        ) noexcept override;

        void resume_on_start_stage() noexcept;
        void resume_on_commands_stage(
            ice::StringID_Arg queue_name,
            ice::gfx::IceGfxQueue* queue
        ) noexcept;
        void resume_on_end_stage() noexcept;

        void execute_final_tasks() noexcept;

    private:
        ice::Allocator& _allocator;
        ice::gfx::GfxTaskCommands* _task_commands;

        ice::Vector<ice::Task<>> _tasks;
        ice::IceshardTaskExecutor _task_executor;

        std::atomic<ice::detail::ScheduleOperationData*> _task_head_start;
        std::atomic<ice::gfx::GfxFrameCommandsOperation::OperationData*> _task_head_commands;
        std::atomic<ice::detail::ScheduleOperationData*> _task_head_end;

        ice::gfx::GfxFrameCommandsOperation::OperationData* _skipped_tasks_temporary = nullptr;
    };

    class IceGfxFrame final : public IceGfxTaskFrame
    {
    public:
        IceGfxFrame(
            ice::Allocator& alloc
        ) noexcept;

        ~IceGfxFrame() noexcept override = default;

        void set_stage_slot(
            ice::gfx::GfxStageSlot slot
        ) noexcept override;

        void set_stage_slots(
            ice::Span<ice::gfx::GfxStageSlot const> slots
        ) noexcept override;

        void enqueue_pass(
            ice::StringID_Arg queue_name,
            ice::gfx::GfxPass* pass
        ) noexcept override;

        void prepare_frame(
            ice::gfx::IceGfxQueueGroup& queue_group
        ) noexcept;

        void execute_passes(
            ice::EngineFrame const& frame,
            ice::gfx::IceGfxQueueGroup& queue_group
        ) noexcept;

    private:
        ice::memory::ProxyAllocator _allocator;

        ice::pod::Hash<ice::gfx::GfxPass*> _enqueued_passes;
        ice::pod::Hash<ice::gfx::GfxStageSlot> _stages;

        // #todo
        ice::gfx::IceGfxQueueGroup* _queue_group;
    };

} // namespace ice::gfx
