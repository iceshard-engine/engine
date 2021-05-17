#include <ice/gfx/gfx_frame.hxx>

namespace ice::gfx
{

    void GfxFrame::push_task_internal(GfxAwaitCommandsOperation* operation) noexcept
    {
        GfxAwaitCommandsOperation* expected_head = _head.load(std::memory_order_acquire);

        do
        {
            operation->_next = expected_head;
        } while (
            _head.compare_exchange_weak(
                expected_head,
                operation,
                std::memory_order_release,
                std::memory_order_acquire
            ) == false
        );
    }

    void GfxFrame::push_task_internal(GfxAwaitFrameEnd* operation) noexcept
    {
        GfxAwaitFrameEnd* expected_head = _frame_end_head.load(std::memory_order_acquire);

        do
        {
            operation->_next = expected_head;
        } while (
            _frame_end_head.compare_exchange_weak(
                expected_head,
                operation,
                std::memory_order_release,
                std::memory_order_acquire
            ) == false
        );
    }

    GfxFrame::GfxAwaitCommandsOperation::GfxAwaitCommandsOperation(
        ice::gfx::GfxFrame& frame,
        ice::StringID_Arg queue_name
    ) noexcept
        : _frame{ frame }
        , _queue{ queue_name }
        , _coro{ nullptr }
        , _next{ nullptr }
    {
    }

    void GfxFrame::GfxAwaitCommandsOperation::await_suspend(std::coroutine_handle<> coro) noexcept
    {
        _coro = coro;
        _frame.push_task_internal(this);
    }

    auto GfxFrame::GfxAwaitCommandsOperation::await_resume() noexcept -> GfxTaskCommands&
    {
        return _frame.task_commands(_queue);
    }

    GfxFrame::GfxAwaitFrameEnd::GfxAwaitFrameEnd(ice::gfx::GfxFrame& frame) noexcept
        : _frame{ frame }
        , _coro{ nullptr }
        , _next{ nullptr }
    {
    }

    void GfxFrame::GfxAwaitFrameEnd::await_suspend(std::coroutine_handle<> coro) noexcept
    {
        _coro = coro;
        _frame.push_task_internal(this);
    }

} // namespace ice::gfx
