#pragma once
#include "task_tracked_promise.hxx"
#include "task_tracked_queue_promise.hxx"

namespace ice
{

    struct TrackedTask
    {
        using PromiseType = ice::TrackedTaskPromise;
    };

    struct TrackedQueue
    {
        using PromiseType = ice::TrackedQueuePromise;
    };

    inline auto ice::TrackedTaskPromise::get_return_object() const noexcept -> ice::TrackedTask
    {
        return {};
    }

    inline auto ice::TrackedQueuePromise::get_return_object() const noexcept -> ice::TrackedQueue
    {
        return {};
    }

} // namespace ice

template<typename... Args>
struct std::coroutine_traits<ice::TrackedTask, Args...>
{
    using promise_type = ice::TrackedTask::PromiseType;
};

// template<typename... Args>
// struct std::coroutine_traits<ice::TrackedQueue, Args...>
// {
//     using promise_type = ice::TrackedQueue::PromiseType;
// };
