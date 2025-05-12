/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task_awaitable.hxx>
#include <ice/task_queue.hxx>
#include <ice/task_stage.hxx>

namespace ice
{

    class TaskScheduler
    {
    public:
        inline explicit TaskScheduler(ice::TaskQueue& queue) noexcept;

        inline auto schedule() noexcept;
        inline auto schedule(ice::TaskFlags flags) noexcept;
        inline auto schedule_delayed(ice::u32 delay_ms) noexcept;

        inline auto operator co_await() noexcept;

    private:
        struct SchedulerAwaitable;

        ice::TaskQueue& _queue;
    };

    struct TaskScheduler::SchedulerAwaitable
    {
        SchedulerAwaitable(
            ice::TaskQueue& queue,
            ice::TaskAwaitableParams params
        ) noexcept
            : _awaitable{ ._params = params }
            , _queue{ queue }
        { }

        bool await_ready() const noexcept
        {
            return false;
        }

        auto await_suspend(ice::coroutine_handle<> coroutine) noexcept
        {
            _awaitable._coro = coroutine;
            _queue.push_back(&_awaitable);
        }

        void await_resume() const noexcept
        {
        }

        ice::TaskAwaitableBase _awaitable;
        ice::TaskQueue& _queue;
    };

    inline TaskScheduler::TaskScheduler(ice::TaskQueue& queue) noexcept
        : _queue{ queue }
    {
    }

    inline auto TaskScheduler::schedule() noexcept
    {
        struct Awaitable : SchedulerAwaitable
        {
            Awaitable(ice::TaskQueue& queue) noexcept
                : SchedulerAwaitable{
                    queue,
                    { .modifier = TaskAwaitableModifier::Unused }
                }
            { }
        };

        return Awaitable{ _queue };
    }

    inline auto TaskScheduler::schedule(ice::TaskFlags flags) noexcept
    {
        struct Awaitable : SchedulerAwaitable
        {
            Awaitable(
                ice::TaskQueue& queue,
                ice::TaskFlags flags
            ) noexcept
                : SchedulerAwaitable{
                    queue,
                    {
                        .modifier = TaskAwaitableModifier::PriorityFlags,
                        .task_flags = flags
                    }
                }
            { }
        };

        return Awaitable{ _queue, flags };
    }

    inline auto TaskScheduler::schedule_delayed(ice::u32 delay_ms) noexcept
    {
        struct Awaitable : SchedulerAwaitable
        {
            Awaitable(
                ice::TaskQueue& queue,
                ice::u32 delay_ms
            ) noexcept
                : SchedulerAwaitable{
                    queue,
                    {
                        .modifier = TaskAwaitableModifier::DelayedExecution,
                        .u32_value = delay_ms
                    }
                }
            { }
        };

        return Awaitable{ _queue, delay_ms };
    }

    inline auto TaskScheduler::operator co_await() noexcept
    {
        return schedule();
    }

} // namespace ice
