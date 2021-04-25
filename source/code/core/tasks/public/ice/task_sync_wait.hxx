#pragma once
#include <ice/task.hxx>
#include <ice/sync_manual_events.hxx>
#include <ice/span.hxx>

namespace ice
{

    auto sync_wait(
        ice::Task<void> task,
        ice::ManualResetEvent& sync_event
    ) noexcept;

    void sync_wait_all(
        ice::Allocator& alloc,
        ice::Span<ice::Task<void>> tasks,
        ice::Span<ice::ManualResetEvent> reset_events
    ) noexcept;

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
