/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task_queue.hxx>
#include <ice/assert.hxx>
#include <coroutine>
#include <atomic>

namespace ice
{

    struct TrackedQueue;

    struct TrackedQueuePromise
    {
        ice::TaskQueue& awaiting_queue;
        std::atomic_uint32_t& awaiting_count;

        inline TrackedQueuePromise(ice::TaskQueue& queue, std::atomic_uint32_t& remaining) noexcept
            : awaiting_queue{ queue }
            , awaiting_count{ remaining }
        { }

        inline TrackedQueuePromise(ice::TaskQueue& queue, std::atomic_uint32_t& remaining, ice::TaskScheduler&) noexcept
            : awaiting_queue{ queue }
            , awaiting_count{ remaining }
        { }

        // Returns 'false' when the current task should run the final continuation
        inline bool notify_completion() noexcept
        {
            awaiting_count.fetch_sub(1, std::memory_order_release);
            return false; // Never ready
            // bool const sdone = awaiting_count.fetch_sub(1, std::memory_order_release) > 1;
            // awaiting_count.notify_one();
            // return sdone;
        }

        inline auto pop() noexcept
        {
            return awaiting_queue.pop();
        }

        struct FinalAwaitable
        {
            TrackedQueuePromise* awaiting_queue;

            inline bool await_ready() const noexcept
            {
                // When all tasks are not yet completed, set this awaitable to 'ready' since it's not going to
                //  to be used to call the awaiting coroutine.
                return awaiting_queue->notify_completion();
            }

            inline auto await_suspend(std::coroutine_handle<> coro) const noexcept -> std::coroutine_handle<>
            {
                std::atomic_thread_fence(std::memory_order_acquire);

                // Can't access promise after it's destroyed.
                std::coroutine_handle<> continuation = coro;

                if (ice::TaskAwaitableBase* awaitable = awaiting_queue->pop(); awaitable != nullptr)
                {
                    // We delete the detached coroutine here, since this would be normaly done when 'await_ready == true' after resuming
                    //  from the final suspension point. Since we are suspending, but we never resume again we just delete it here.
                    coro.destroy();

                    // Update the continuation to the accessed awaitable.
                    continuation = awaitable->_coro;
                }

                // Return the continuation
                return continuation;
            }

            inline void await_resume() const noexcept
            {
            }
        };

        inline auto initial_suspend() const noexcept { return std::suspend_never{ }; }
        inline auto final_suspend() noexcept { return FinalAwaitable{ this }; }
        inline auto return_void() const noexcept { }

        inline auto get_return_object() const noexcept -> ice::TrackedQueue;
        inline void unhandled_exception() const noexcept
        {
            ICE_ASSERT(false, "Unexpected coroutine exception!");
        }
    };

} // namespace ice
