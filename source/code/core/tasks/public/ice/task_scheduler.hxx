/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task.hxx>

namespace ice
{

    struct TaskOperation_v2
    {
        std::coroutine_handle<> _coro;
        ice::TaskOperation_v2* _next;
    };

    class TaskScheduler_v2
    {
    public:
        virtual ~TaskScheduler_v2() noexcept = default;

        virtual bool schedule(TaskOperation_v2& operation) noexcept = 0;
    };

    namespace detail
    {

        struct TaskOperation_Internal : TaskOperation_v2
        {
            inline bool await_ready() const noexcept { return false; }
            inline bool await_suspend(std::coroutine_handle<void> coro) noexcept;
            inline void await_resume() const noexcept { }

            ice::TaskScheduler_v2* _scheduler;
        };

        inline bool TaskOperation_Internal::await_suspend(std::coroutine_handle<void> coro) noexcept
        {
            _coro = coro;
            _next = nullptr;
            return _scheduler->schedule(*this);
        }

    } // namespace detail

    inline auto operator co_await(TaskScheduler_v2& scheduler) noexcept -> ice::detail::TaskOperation_Internal
    {
        return ice::detail::TaskOperation_Internal{ ._scheduler = &scheduler };
    }

} // namespace ice
