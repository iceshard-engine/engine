#include <ice/task_checkpoint.hxx>

namespace ice
{

    TaskCheckpoint::TaskCheckpoint(bool initially_open) noexcept
        : _checkpoint_state{ initially_open ? ice::i32_min : 0 }
        , _checkpoint_queue{ }
    {
    }

    bool TaskCheckpoint::open(ice::TaskScheduler* scheduler, void* result_ptr) noexcept
    {
        ice::i32 const awaiting_count = _checkpoint_state.exchange(ice::i32_min);
        if (awaiting_count > 0)
        {
            if (scheduler == nullptr)
            {
                ice::u32 const processed_count = _checkpoint_queue.process_all(result_ptr);
                ICE_ASSERT_CORE(ice::i32(processed_count) == awaiting_count);
            }
            else if (result_ptr == nullptr)
            {
                ice::schedule_queue(_checkpoint_queue, *scheduler);
            }
            else
            {
                ice::schedule_queue(_checkpoint_queue, result_ptr, *scheduler);
            }
        }
        return awaiting_count > 0;
    }

    auto TaskCheckpoint::opened() noexcept -> ice::TaskCheckpointGate
    {
        return ice::TaskCheckpointGate{ _checkpoint_state, _checkpoint_queue };
    }

    void TaskCheckpoint::close() noexcept
    {
        _checkpoint_state.exchange(0);
    }

} // namespace ice
