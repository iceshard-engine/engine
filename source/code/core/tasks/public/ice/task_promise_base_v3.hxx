#pragma once
#include <ice/task_types_v3.hxx>

namespace ice
{

    struct TaskPromiseBase
    {
        struct FinalAwaitable
        {
            inline bool await_ready() const noexcept;

            template<typename Promise>
            inline auto await_suspend(ice::coroutine_handle<Promise> coro) noexcept -> ice::coroutine_handle<>;

            inline void await_resume() const noexcept;
        };

        inline auto initial_suspend() const noexcept;

        inline auto final_suspend() const noexcept;

        inline auto set_continuation(ice::coroutine_handle<> coro) noexcept;

        inline auto continuation() const noexcept -> ice::coroutine_handle<>;

        inline void unhandled_exception() const noexcept;

    protected:
        inline TaskPromiseBase() noexcept = default;

    private:
        ice::coroutine_handle<> _continuation;
    };

    inline bool TaskPromiseBase::FinalAwaitable::await_ready() const noexcept
    {
        return false;
    }

    template<typename Promise>
    inline auto TaskPromiseBase::FinalAwaitable::await_suspend(ice::coroutine_handle<Promise> coro) noexcept -> ice::coroutine_handle<>
    {
        return coro.promise().continuation();
    }

    inline void TaskPromiseBase::FinalAwaitable::await_resume() const noexcept
    {
    }

    inline auto TaskPromiseBase::initial_suspend() const noexcept
    {
        return ice::suspend_always{ };
    }

    inline auto TaskPromiseBase::final_suspend() const noexcept
    {
        return FinalAwaitable{ };
    }

    inline auto TaskPromiseBase::set_continuation(ice::coroutine_handle<> coro) noexcept
    {
        _continuation = coro;
    }

    inline auto TaskPromiseBase::continuation() const noexcept -> ice::coroutine_handle<>
    {
        return _continuation;
    }

    inline void TaskPromiseBase::unhandled_exception() const noexcept
    {
        ICE_ASSERT(false, "Unexpected exception in task promise! IceShard does not support exceptions!");
    }

} // namespace ice
