#pragma once
#include <ice/gfx/gfx_frame.hxx>
#include <ice/gfx/gfx_queue.hxx>
#include <ice/gfx/gfx_task.hxx>

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

    using GfxCmdOperation = ice::gfx::GfxFrameCommandsOperation::OperationData;

    class IceGfxTaskFrame : public ice::gfx::GfxFrame
    {
    public:
        IceGfxTaskFrame(ice::Allocator& alloc) noexcept;
        ~IceGfxTaskFrame() noexcept override = default;

        void execute_task(ice::Task<> task) noexcept;

        void query_operations(
            ice::Span<ice::StringID_Hash const> names,
            ice::Span<GfxCmdOperation*> operation_heads
        ) noexcept;

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

        auto create_task_executor() noexcept -> ice::IceshardTaskExecutor;

        void resume_on_start_stage() noexcept;
        void resume_on_end_stage() noexcept;

    private:
        ice::Allocator& _allocator;
        ice::gfx::GfxTaskCommands* _task_commands;

        ice::Vector<ice::Task<>> _tasks;

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
            ice::StringID_Arg stage_name,
            ice::gfx::GfxStage* stage
        ) noexcept override;

        void enqueue_pass(
            ice::StringID_Arg queue_name,
            ice::gfx::GfxPass* pass
        ) noexcept override;

        bool query_queue_stages(
            ice::StringID_Arg queue_name,
            ice::pod::Array<ice::gfx::GfxStage*>& out_stages
        ) noexcept;

        void execute_passes(
            ice::EngineFrame const& frame,
            ice::gfx::IceGfxQueue& queue
        ) noexcept;

    private:
        ice::Allocator& _allocator;

        ice::pod::Hash<ice::gfx::GfxPass*> _enqueued_passes;
        ice::pod::Hash<ice::gfx::GfxStage*> _stages;

        ice::gfx::IceGfxQueueGroup* _queue_group;
    };

    class IceGfxPassExecutor : public ice::gfx::GfxTaskCommands
    {
    public:
        IceGfxPassExecutor(
            ice::render::RenderFence const& fence,
            ice::gfx::IceGfxQueue* queue,
            ice::gfx::GfxCmdOperation* operations,
            ice::pod::Array<ice::gfx::GfxStage*> stages
        ) noexcept;

        void record(ice::EngineFrame const& frame, ice::render::RenderCommands& api) noexcept;

        void execute() noexcept;

    protected:
        void update_texture(
            ice::render::Image image,
            ice::render::Buffer image_contents,
            ice::vec2u extents
        ) noexcept override;

    private:
        ice::render::RenderFence const& _fence;
        ice::gfx::IceGfxQueue* const _queue;
        ice::render::CommandBuffer _command_buffer;

        ice::gfx::GfxCmdOperation* _operations;
        ice::pod::Array<ice::gfx::GfxStage*> _stages;

        // HACKS:
        ice::render::RenderCommands* _api = nullptr;
    };

    auto create_pass_executor(
        ice::Allocator& _allocator,
        ice::render::RenderFence const& fence,
        ice::render::QueueFlags flags,
        ice::gfx::IceGfxQueueGroup& queue_group,
        ice::gfx::IceGfxFrame& gfx_frame,
        ice::Span<ice::StringID_Hash> names,
        ice::Span<ice::gfx::GfxCmdOperation*> operations
    ) noexcept -> ice::UniquePtr<IceGfxPassExecutor>;

} // namespace ice::gfx
