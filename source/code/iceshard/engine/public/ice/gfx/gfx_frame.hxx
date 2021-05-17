#pragma once
#include <ice/stringid.hxx>
#include <ice/task.hxx>
#include <ice/render/render_command_buffer.hxx>
#include <ice/gfx/gfx_pass.hxx>

namespace ice::gfx
{

    class GfxTaskCommands;

    class GfxFrame
    {
    protected:
        virtual ~GfxFrame() noexcept = default;

    public:
        struct GfxAwaitCommandsOperation;
        struct GfxAwaitFrameEnd;

        virtual auto aquire_task_commands(
            ice::StringID_Arg queue_name
        ) noexcept -> GfxAwaitCommandsOperation = 0;

        virtual auto frame_end() noexcept -> GfxAwaitFrameEnd = 0;

        virtual void execute_task(
            ice::Task<void> task
        ) noexcept = 0;

        virtual void enqueue_pass(
            ice::StringID_Arg queue_name,
            ice::gfx::GfxPass* pass
        ) noexcept = 0;

    protected:
        virtual auto task_commands(
            ice::StringID_Arg queue_name
        ) noexcept -> GfxTaskCommands& = 0;

        void push_task_internal(GfxAwaitCommandsOperation* operation) noexcept;
        void push_task_internal(GfxAwaitFrameEnd* operation) noexcept;

    protected:
        std::atomic<GfxAwaitCommandsOperation*> _head = nullptr;
        std::atomic<GfxAwaitFrameEnd*> _frame_end_head = nullptr;
    };

    struct GfxFrame::GfxAwaitCommandsOperation
    {
    public:
        GfxAwaitCommandsOperation(
            ice::gfx::GfxFrame& frame,
            ice::StringID_Arg queue_name
        ) noexcept;

        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> coro) noexcept;
        auto await_resume() noexcept -> GfxTaskCommands&;

    private:
        friend class GfxFrame;
        friend class IceGfxFrame;

        ice::gfx::GfxFrame& _frame;
        ice::StringID _queue;
        std::coroutine_handle<> _coro;
        GfxAwaitCommandsOperation* _next;
    };

    struct GfxFrame::GfxAwaitFrameEnd
    {
    public:
        friend class GfxFrame;
        friend class IceGfxFrame;

        GfxAwaitFrameEnd(
            ice::gfx::GfxFrame& frame
        ) noexcept;

        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> coro) noexcept;
        void await_resume() noexcept { }

    private:
        friend class GfxFrame;
        friend class IceGfxFrame;

        ice::gfx::GfxFrame& _frame;
        std::coroutine_handle<> _coro;
        GfxAwaitFrameEnd* _next;
    };

} // namespace ice::gfx
