/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task_types.hxx>
#include <ice/task_awaitable.hxx>
#include <ice/task_queue.hxx>
#include <ice/task_utils.hxx>
#include <ice/task.hxx>
#include <atomic>

namespace ice
{

    class TaskCheckpoint
    {
    public:
        TaskCheckpoint(bool initially_open = false) noexcept;

        bool is_open() const noexcept;

        bool open() noexcept;
        bool open(ice::TaskScheduler& scheduler) noexcept;
        void close() noexcept;

        auto checkpoint_gate() noexcept -> ice::TaskCheckpointGate;
        bool enqueue_awaitable(ice::TaskAwaitableBase& awaitable) noexcept;

    private:
        std::atomic_int32_t _checkpoint_state;
        ice::TaskQueue _checkpoint_queue;
    };

    class TaskCheckpointGate
    {
    public:
        TaskCheckpointGate() noexcept;
        TaskCheckpointGate(ice::TaskCheckpoint& checkpoint) noexcept;

        TaskCheckpointGate(ice::TaskCheckpointGate const& other) noexcept;
        auto operator=(ice::TaskCheckpointGate const& other) noexcept -> ice::TaskCheckpointGate&;

        inline bool await_ready() const noexcept;

        inline bool await_suspend(std::coroutine_handle<> coro) noexcept;

        inline void await_resume() const noexcept { }

    private:
        ice::TaskAwaitableBase _awaitable;
        ice::TaskCheckpoint* _checkpoint;
    };

    inline bool TaskCheckpointGate::await_ready() const noexcept
    {
        return _checkpoint == nullptr || _checkpoint->is_open();
    }

    inline bool TaskCheckpointGate::await_suspend(std::coroutine_handle<> coro) noexcept
    {
        _awaitable._coro = coro;
        return _checkpoint->enqueue_awaitable(_awaitable);
    }

} // namespace ice
