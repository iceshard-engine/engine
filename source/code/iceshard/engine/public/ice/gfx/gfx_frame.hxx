#pragma once
#include <ice/stringid.hxx>
#include <ice/task.hxx>
#include <ice/task_operations.hxx>
#include <ice/render/render_command_buffer.hxx>
#include <ice/gfx/gfx_pass.hxx>
#include <ice/gfx/gfx_stage.hxx>

namespace ice::gfx
{

    class GfxStage;
    class GfxTaskFrame;
    class GfxTaskCommands;

    class GfxFrameStartOperation : public ice::ScheduleOperation<ice::gfx::GfxTaskFrame>
    {
    public:
        using ScheduleOperation::ScheduleOperation;

        inline void await_suspend(std::coroutine_handle<void> coro) noexcept;
    };

    class GfxFrameCommandsOperation
    {
    public:
        struct OperationData
        {
            std::coroutine_handle<> _coroutine = nullptr;
            OperationData* _next = nullptr;
            ice::StringID_Hash _queue_name;
            ice::gfx::GfxTaskCommands* _commands;
        };

        using DataMemberType = OperationData GfxFrameCommandsOperation::*;

        inline GfxFrameCommandsOperation(
            ice::gfx::GfxTaskFrame& task_frame,
            ice::StringID_Arg queue_name
        ) noexcept;

        inline bool await_ready() const noexcept { return false; }
        inline void await_suspend(std::coroutine_handle<void> coro) noexcept;
        inline auto await_resume() const noexcept -> ice::gfx::GfxTaskCommands&;

    private:
        ice::gfx::GfxTaskFrame& _target;
        OperationData _data;
    };

    inline GfxFrameCommandsOperation::GfxFrameCommandsOperation(
        ice::gfx::GfxTaskFrame& task_frame,
        ice::StringID_Arg queue_name
    ) noexcept
        : _target{ task_frame }
        , _data{ ._queue_name = ice::stringid_hash(queue_name) }
    { }

    class GfxFrameEndOperation : public ice::ScheduleOperation<ice::gfx::GfxTaskFrame>
    {
    public:
        using ScheduleOperation::ScheduleOperation;

        inline void await_suspend(std::coroutine_handle<void> coro) noexcept;
    };


    class GfxTaskFrame
    {
    public:
        virtual ~GfxTaskFrame() noexcept = default;

        [[nodiscard]]
        virtual auto frame_start() noexcept -> ice::gfx::GfxFrameStartOperation = 0;

        [[nodiscard]]
        virtual auto frame_commands(ice::StringID_Arg queue_name) noexcept -> ice::gfx::GfxFrameCommandsOperation = 0;

        [[nodiscard]]
        virtual auto frame_end() noexcept -> ice::gfx::GfxFrameEndOperation = 0;

    public:
        virtual void schedule_internal(
            ice::gfx::GfxFrameStartOperation* operation,
            ice::gfx::GfxFrameStartOperation::DataMemberType data
        ) noexcept = 0;

        virtual void schedule_internal(
            ice::gfx::GfxFrameCommandsOperation* operation,
            ice::gfx::GfxFrameCommandsOperation::DataMemberType data
        ) noexcept = 0;

        virtual void schedule_internal(
            ice::gfx::GfxFrameEndOperation* operation,
            ice::gfx::GfxFrameEndOperation::DataMemberType data
        ) noexcept = 0;
    };


    inline void GfxFrameStartOperation::await_suspend(std::coroutine_handle<void> coro) noexcept
    {
        _data._coroutine = coro;
        _target.schedule_internal(this, &GfxFrameStartOperation::_data);
    }

    inline void GfxFrameCommandsOperation::await_suspend(std::coroutine_handle<void> coro) noexcept
    {
        _data._coroutine = coro;
        _target.schedule_internal(this, &GfxFrameCommandsOperation::_data);
    }

    inline auto GfxFrameCommandsOperation::await_resume() const noexcept -> ice::gfx::GfxTaskCommands&
    {
        return *_data._commands;
    }

    inline void GfxFrameEndOperation::await_suspend(std::coroutine_handle<void> coro) noexcept
    {
        _data._coroutine = coro;
        _target.schedule_internal(this, &GfxFrameEndOperation::_data);
    }


    class GfxFrame : public ice::gfx::GfxTaskFrame
    {
    protected:
        virtual ~GfxFrame() noexcept = default;

    public:
        virtual void set_stage_slot(
            ice::StringID_Arg stage_name,
            ice::gfx::GfxStage* stage
        ) noexcept = 0;

        virtual void enqueue_pass(
            ice::StringID_Arg queue_name,
            ice::gfx::GfxPass* pass
        ) noexcept = 0;
    };

} // namespace ice::gfx
