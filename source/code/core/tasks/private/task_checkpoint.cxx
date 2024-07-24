#include <ice/task_checkpoint.hxx>

namespace ice
{

    TaskCheckpointGate::TaskCheckpointGate() noexcept
        : _awaitable{ ._params = { .modifier = TaskAwaitableModifier::Unused } }
        , _checkpoint{ nullptr }
    {
    }

    TaskCheckpointGate::TaskCheckpointGate(ice::TaskCheckpoint& checkpoint) noexcept
        : _awaitable{ ._params = { .modifier = TaskAwaitableModifier::Unused } }
        , _checkpoint{ ice::addressof(checkpoint) }
    {
    }

    TaskCheckpointGate::TaskCheckpointGate(ice::TaskCheckpointGate const& other) noexcept
        : _awaitable{ ._params = { .modifier = TaskAwaitableModifier::Unused } }
        , _checkpoint{ other._checkpoint }
    {
    }

    auto TaskCheckpointGate::operator=(ice::TaskCheckpointGate const& other) noexcept -> ice::TaskCheckpointGate&
    {
        if (ice::addressof(other) != this)
        {
            _checkpoint = other._checkpoint;
        }
        return *this;
    }

    TaskCheckpoint::TaskCheckpoint(bool initially_open) noexcept
        : _checkpoint_state{ initially_open ? ice::i32_min : 0 }
        , _checkpoint_queue{ }
    {
    }

    bool TaskCheckpoint::is_open() const noexcept
    {
        return _checkpoint_state.load(std::memory_order_relaxed) < 0;
    }

    bool TaskCheckpoint::open() noexcept
    {
        // We use release memory ordering to ensure the _checkpoint_result is visible for threads that would see the state after the exchange.
        ice::i32 const awaiting_count = _checkpoint_state.exchange(ice::i32_min, std::memory_order_release);
        ICE_ASSERT_CORE(awaiting_count >= 0);
        if (awaiting_count > 0)
        {
            ice::u32 const processed_count = _checkpoint_queue.process_all();
            ICE_ASSERT_CORE(ice::i32(processed_count) == awaiting_count);
        }
        return awaiting_count > 0;
    }

    bool TaskCheckpoint::open(ice::TaskScheduler& scheduler) noexcept
    {
        // We use release memory ordering to ensure the _checkpoint_result is visible for threads that would see the state after the exchange.
        ice::i32 const awaiting_count = _checkpoint_state.exchange(ice::i32_min, std::memory_order_release);
        if (awaiting_count > 0)
        {
            ice::schedule_queue(_checkpoint_queue, scheduler);
        }
        return awaiting_count > 0;
    }

    void TaskCheckpoint::close() noexcept
    {
        _checkpoint_state.exchange(0, std::memory_order_relaxed);
    }

    auto TaskCheckpoint::checkpoint_gate() noexcept -> ice::TaskCheckpointGate
    {
        return ice::TaskCheckpointGate{ *this };
    }

    bool TaskCheckpoint::enqueue_awaitable(ice::TaskAwaitableBase& awaitable) noexcept
    {
        // Queue has release memory semantics when pushing new awaitables.
        ice::i32 const state = _checkpoint_state.fetch_add(1, std::memory_order_relaxed);
        if (state >= 0)
        {
            _checkpoint_queue.push_back(ice::addressof(awaitable));
        }
        return state >= 0;
    }

} // namespace ice
