#pragma once
#include <ice/task_types_v3.hxx>
#include <ice/task_awaitable_v3.hxx>
#include <ice/task_queue_v3.hxx>

namespace ice
{

    template<typename Value>
    auto schedule_on(ice::Task_v3<Value>&& task, ice::TaskScheduler_v3& scheduler) noexcept -> ice::Task_v3<Value>;

    template<typename Value>
    auto resume_on(ice::Task_v3<Value>&& task, ice::TaskScheduler_v3& scheduler) noexcept -> ice::Task_v3<Value>;

    class TaskScheduler_v3
    {
    public:
        inline explicit TaskScheduler_v3(ice::TaskQueue_v3& queue) noexcept;

        inline auto schedule() noexcept;
        inline auto schedule(ice::TaskFlags flags) noexcept;
        inline auto schedule_delayed(ice::u32 delay_ms) noexcept;

        inline auto operator co_await() noexcept;

    private:
        struct SchedulerAwaitable;

        ice::TaskQueue_v3& _queue;
    };


    struct TaskScheduler_v3::SchedulerAwaitable : TaskAwaitableBase_v3
    {
        SchedulerAwaitable(
            ice::TaskQueue_v3& queue,
            ice::TaskAwaitableParams_v3 params
        ) noexcept
            : TaskAwaitableBase_v3{ ._params = params }
            , _queue{ queue }
        { }

        auto await_suspend(ice::coroutine_handle<> coroutine) noexcept
        {
            _coro = coroutine;
            ice::linked_queue::push(_queue._awaitables, this);
        }

        ice::TaskQueue_v3& _queue;
    };

    inline TaskScheduler_v3::TaskScheduler_v3(ice::TaskQueue_v3& queue) noexcept
        : _queue{ queue }
    {
    }

    inline auto TaskScheduler_v3::schedule() noexcept
    {
        struct Awaitable : SchedulerAwaitable
        {
            Awaitable(ice::TaskQueue_v3& queue) noexcept
                : SchedulerAwaitable{
                    queue,
                    { .modifier = TaskAwaitableModifier_v3::None }
                }
            { }
        };

        return Awaitable{ _queue };
    }

    inline auto TaskScheduler_v3::schedule(ice::TaskFlags flags) noexcept
    {
        struct Awaitable : SchedulerAwaitable
        {
            Awaitable(
                ice::TaskQueue_v3& queue,
                ice::TaskFlags flags
            ) noexcept
                : SchedulerAwaitable{
                    queue,
                    {
                        .modifier = TaskAwaitableModifier_v3::PriorityFlags,
                        .task_flags = flags
                    }
                }
            { }
        };

        return Awaitable{ _queue, flags };
    }

    inline auto TaskScheduler_v3::schedule_delayed(ice::u32 delay_ms) noexcept
    {
        struct Awaitable : SchedulerAwaitable
        {
            Awaitable(
                ice::TaskQueue_v3& queue,
                ice::u32 delay_ms
            ) noexcept
                : SchedulerAwaitable{
                    queue,
                    {
                        .modifier = TaskAwaitableModifier_v3::DelayedExecution,
                        .u32_delay = delay_ms
                    }
                }
            { }
        };

        return Awaitable{ _queue, delay_ms };
    }

    inline auto TaskScheduler_v3::operator co_await() noexcept
    {
        return schedule();
    }

    template<typename Value>
    auto schedule_on(ice::Task_v3<Value>&& task, ice::TaskScheduler_v3& scheduler) noexcept -> ice::Task_v3<Value>
    {
        co_await scheduler;
        co_return co_await std::move(task);
    }

    template<typename Value>
    auto resume_on(ice::Task_v3<Value>&& task, ice::TaskScheduler_v3& scheduler) noexcept -> ice::Task_v3<Value>
    {
        Value value = co_await std::move(task);
        co_await scheduler;
        co_return std::move(value);
    }

} // namespace ice
