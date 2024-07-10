#pragma once
#include <ice/task_awaitable.hxx>
#include <ice/task_queue.hxx>
#include <ice/task_utils.hxx>
#include <ice/task.hxx>
#include <atomic>

namespace ice
{

    struct TaskCheckpointGate
    {
        std::atomic_int32_t& _reached;
        ice::TaskQueue& _queue;
        ice::TaskAwaitableBase _awaitable{ ._params = { .modifier = TaskAwaitableModifier::Unused } };

        inline bool await_ready() const noexcept
        {
            return _reached.load(std::memory_order_relaxed) < 0;
        }

        inline auto await_suspend(std::coroutine_handle<> coro) noexcept -> std::coroutine_handle<>
        {
            if (_reached.fetch_add(1, std::memory_order_release) >= 0)
            {
                _awaitable._coro = std::exchange(coro, std::noop_coroutine());
                _queue.push_back(&_awaitable);
            }
            return coro;
        }

        inline void await_resume() const noexcept
        {
            std::atomic_thread_fence(std::memory_order_acquire);
        }
    };

    struct TaskCheckpoint
    {
        TaskCheckpoint(bool initially_open = false) noexcept;

        bool open(ice::TaskScheduler* scheduler = nullptr, void* result_ptr = nullptr) noexcept;
        auto opened() noexcept -> ice::TaskCheckpointGate;
        void close() noexcept;

        std::atomic_int32_t _checkpoint_state;
        ice::TaskQueue _checkpoint_queue;
    };

#if 0
    struct TaskCheckpoint
    {
        ice::TaskScheduler& _scheduler;
        ice::TaskQueue _queue;
        std::atomic_flag _reached = ATOMIC_FLAG_INIT;

        struct ReachedReset
        {
            std::atomic_flag* _reached_ref = nullptr;

            ReachedReset() noexcept = default;
            ReachedReset(ReachedReset&& other) noexcept : _reached_ref{ std::exchange(other._reached_ref, nullptr) } { }
            auto& operator=(ReachedReset&& other) noexcept { _reached_ref = std::exchange(other._reached_ref, nullptr); return *this; }
            ReachedReset(std::atomic_flag& ref) noexcept : _reached_ref{ &ref } { }
            ~ReachedReset() noexcept { if (_reached_ref) _reached_ref->clear(); }
        };

        [[nodiscard]]
        auto reached() noexcept -> ice::Task<ReachedReset>
        {
            _reached.test_and_set(std::memory_order_relaxed); // Set the reached flag
            co_await ice::await_scheduled_queue(_queue, _scheduler);
            co_return ReachedReset{ _reached };
        };

        auto operator co_await() noexcept
        {
            struct CheckpointAwaitable
            {
                std::atomic_flag& _reached_ref;
                ice::TaskQueue& _queue;
                ice::TaskAwaitableBase _awaitable;

                inline bool await_ready() const noexcept
                {
                    return _reached_ref.test(std::memory_order_acquire);
                }

                inline bool await_suspend(std::coroutine_handle<> coro) noexcept
                {
                    _awaitable._coro = coro;
                    // ice::linked_queue::push(_queue._awaitables, &_awaitable);

                    // Check one more time just to make sure.
                    //  if it was set, we might have already missed the queue consume on the 'reached' call.
                    if (_reached_ref.test(std::memory_order_acquire))
                    {
                        return _queue.contains(ice::addressof(_awaitable));
                    }
                    return true;
                }

                constexpr void await_resume() const noexcept
                {
                }
            };
            return CheckpointAwaitable{_reached, _queue};
        }
    };
#endif

} // namespace ice
