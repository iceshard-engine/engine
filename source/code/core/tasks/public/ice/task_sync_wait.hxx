#pragma once
#include <ice/task.hxx>
#include <ice/sync_manual_events.hxx>
#include <ice/span.hxx>

namespace ice
{

    void sync_wait(
        ice::Task<void> task
    ) noexcept;

    template<typename T>
    auto sync_wait(
        ice::Task<T> task
    ) noexcept -> T
    {
        T result;
        auto const task_wrapper = [](T& result, ice::Task<T> awaited_task) noexcept -> ice::Task<>
        {
            result = co_await awaited_task;
        };

        ice::sync_wait(task_wrapper(result, ice::move(task)));
        return result;
    }

    void sync_manual_wait(
        ice::Task<void> task,
        ice::ManualResetEvent& reset_event
    ) noexcept;

    void sync_wait_all(
        ice::Allocator& alloc,
        ice::Span<ice::Task<void>> tasks,
        ice::Span<ice::ManualResetEvent> reset_events
    ) noexcept;

    void when_all_ready(
        //ice::Allocator& alloc,
        ice::Span<ice::Task<void>> tasks,
        ice::Span<ice::ManualResetEvent> reset_events
    ) noexcept;

    auto sync_task(
        ice::Task<void> task,
        ice::ManualResetEvent* reset_event
    ) noexcept -> ice::Task<>;

    //template<typename T>
    //auto sync_wait(ice::Task<T> task) noexcept;

    //namespace detail
    //{

    //    template<typename T>
    //    struct SyncWaitPromise
    //    {

    //    };

    //    template<typename T>
    //    class SyncWaitTask
    //    {
    //        using PromiseType = SyncWaitPromise;
    //    public:
    //        SyncWaitTask(std::coroutine_handle<PromiseType> coro) noexcept;


    //    private:
    //        std::coroutine_handle<PromiseType> _coroutine;

    //        using promise_type = PromiseType;
    //    };

    //} // namespace detail

    //template<typename T>
    //auto sync_wait(ice::Task<T> task) noexcept
    //{
    //    ice::ManualResetEvent sync_event;

    //}

} // namespace ice
