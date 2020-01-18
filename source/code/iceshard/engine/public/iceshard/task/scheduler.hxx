#pragma once
#include <cppcoro/task.hpp>

namespace iceshard
{

    struct Scheduler
    {
        friend struct Awaitable;

        auto schedule() noexcept -> Awaitable;

    protected:
        virtual void schedule_coroutine_handle(std::experimental::coroutine_handle<> coroutine_handle) noexcept = 0;
    };

    struct Awaitable
    {
        Awaitable(Scheduler& scheduler) noexcept;

        bool await_ready() noexcept { return false; }

        auto await_suspend(std::experimental::coroutine_handle<> coroutine_handle) noexcept
        {
            _scheduler.schedule_coroutine_handle(coroutine_handle);
        }

        void await_resume() noexcept {}

    private:
        Scheduler& _scheduler;
    };

    inline auto Scheduler::schedule() noexcept -> Awaitable
    {
        return Awaitable{ *this };
    }

    inline Awaitable::Awaitable(Scheduler& scheduler) noexcept
        : _scheduler{ scheduler }
    {
    }

} // namespace iceshard
