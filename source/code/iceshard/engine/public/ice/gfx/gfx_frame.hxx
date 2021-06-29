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
            ice::StringID_Hash queue_name;
        };

        using DataMemberType = OperationData GfxFrameCommandsOperation::*;

        inline GfxFrameCommandsOperation(
            ice::gfx::GfxTaskFrame& task_frame,
            ice::gfx::GfxTaskCommands*& task_commands,
            ice::StringID_Arg queue_name
        ) noexcept;

        inline bool await_ready() const noexcept { return false; }
        inline void await_suspend(std::coroutine_handle<void> coro) noexcept;
        inline auto await_resume() const noexcept -> ice::gfx::GfxTaskCommands&;

    private:
        ice::gfx::GfxTaskFrame& _target;
        ice::gfx::GfxTaskCommands*& _task_commands;
        OperationData _data;
    };

    inline GfxFrameCommandsOperation::GfxFrameCommandsOperation(
        ice::gfx::GfxTaskFrame& task_frame,
        ice::gfx::GfxTaskCommands*& task_commands,
        ice::StringID_Arg queue_name
    ) noexcept
        : _target{ task_frame }
        , _task_commands{ task_commands }
        , _data{ .queue_name = ice::stringid_hash(queue_name) }
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
        return *_task_commands;
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
    //    struct GfxAwaitCommandsOperation;
    //    struct GfxAwaitFrameEnd;

        //virtual auto aquire_task_commands(
        //    ice::StringID_Arg queue_name
        //) noexcept -> GfxAwaitCommandsOperation = 0;

        //virtual auto frame_end() noexcept -> GfxAwaitFrameEnd = 0;

        //virtual void execute_task(
        //    ice::Task<void> task
        //) noexcept = 0;
        virtual void set_stage_slot(
            ice::gfx::GfxStageSlot slot
        ) noexcept = 0;

        virtual void set_stage_slots(
            ice::Span<ice::gfx::GfxStageSlot const> slots
        ) noexcept = 0;

        virtual void enqueue_pass(
            ice::StringID_Arg queue_name,
            ice::gfx::GfxPass* pass
        ) noexcept = 0;

    protected:
        //virtual auto task_commands(
        //    ice::StringID_Arg queue_name
        //) noexcept -> GfxTaskCommands& = 0;

    //    void push_task_internal(GfxAwaitCommandsOperation* operation) noexcept;
    //    void push_task_internal(GfxAwaitFrameEnd* operation) noexcept;

    //protected:
    //    std::atomic<GfxAwaitCommandsOperation*> _head = nullptr;
    //    std::atomic<GfxAwaitFrameEnd*> _frame_end_head = nullptr;
    };

    //struct GfxFrame::GfxAwaitCommandsOperation
    //{
    //public:
    //    GfxAwaitCommandsOperation(
    //        ice::gfx::GfxFrame& frame,
    //        ice::StringID_Arg queue_name
    //    ) noexcept;

    //    bool await_ready() const noexcept { return false; }
    //    void await_suspend(std::coroutine_handle<> coro) noexcept;
    //    auto await_resume() noexcept -> GfxTaskCommands&;

    //private:
    //    friend class GfxFrame;
    //    friend class IceGfxFrame;

    //    ice::gfx::GfxFrame& _frame;
    //    ice::StringID _queue;
    //    std::coroutine_handle<> _coro;
    //    GfxAwaitCommandsOperation* _next;
    //};

    //struct GfxFrame::GfxAwaitFrameEnd
    //{
    //public:
    //    friend class GfxFrame;
    //    friend class IceGfxFrame;

    //    GfxAwaitFrameEnd(
    //        ice::gfx::GfxFrame& frame
    //    ) noexcept;

    //    bool await_ready() const noexcept { return false; }
    //    void await_suspend(std::coroutine_handle<> coro) noexcept;
    //    void await_resume() noexcept { }

    //private:
    //    friend class GfxFrame;
    //    friend class IceGfxFrame;

    //    ice::gfx::GfxFrame& _frame;
    //    std::coroutine_handle<> _coro;
    //    GfxAwaitFrameEnd* _next;
    //};

} // namespace ice::gfx
