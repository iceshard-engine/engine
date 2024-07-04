#pragma once
#include <ice/task.hxx>
#include <coroutine>
#include <atomic>

namespace ice
{

    struct TrackedTask;

    struct TrackedTaskPromise
    {
        ~TrackedTaskPromise() noexcept { }

        std::coroutine_handle<> awaiting_continuation;
        std::atomic_uint32_t& awaiting_count;

        inline TrackedTaskPromise(ice::Task<> const&, std::coroutine_handle<> continuation, std::atomic_uint32_t& remaining) noexcept
            : awaiting_continuation{ continuation }
            , awaiting_count{ remaining }
        {
        }

        inline TrackedTaskPromise(ice::Task<> const&, std::coroutine_handle<> continuation, std::atomic_uint32_t& remaining, ice::TaskScheduler&) noexcept
            : awaiting_continuation{ continuation }
            , awaiting_count{ remaining }
        {
        }

        // Returns 'false' when the current task should run the final continuation
        inline bool notify_completion() noexcept
        {
            bool const sdone = awaiting_count.fetch_sub(1, std::memory_order_release) > 1;
            awaiting_count.notify_one();
            return sdone;
        }

        struct FinalAwaitable
        {
            TrackedTaskPromise* awaiting_task;
            std::coroutine_handle<> awaiting_destruct;

            ~FinalAwaitable() noexcept
            {
                if (awaiting_destruct)
                {
                    awaiting_destruct.destroy();
                }
            }

            inline bool await_ready() const noexcept
            {
                // When all tasks are not yet completed, set this awaitable to 'ready' since it's not going to
                //  to be used to call the awaiting coroutine.
                return awaiting_task->notify_completion();
            }

            inline auto await_suspend(std::coroutine_handle<> coro) const noexcept -> std::coroutine_handle<>
            {
                std::atomic_thread_fence(std::memory_order_acquire);

                // Can't access promise after it's destroyed
                std::coroutine_handle<> continuation = awaiting_task->awaiting_continuation;

                // We delete the detached coroutine here, since this would be normaly done when 'await_ready == true' after resuming
                //  from the final suspension point. Since we are suspending, but we never resume again we just delete it here.
                coro.destroy();

                continuation.resume();

                // Return the continuation
                return std::noop_coroutine();
            }

            inline void await_resume() const noexcept
            {
                // Can resume if 'await_read' returned true
            }
        };

        inline auto initial_suspend() const noexcept { return ice::suspend_never{ }; }
        inline auto final_suspend() noexcept { return FinalAwaitable{ this }; }
        inline auto return_void() const noexcept { }

        inline auto get_return_object() const noexcept -> ice::TrackedTask;
        inline void unhandled_exception() const noexcept
        {
            ICE_ASSERT(false, "Unexpected coroutine exception!");
        }
    };

} // namespace ice
