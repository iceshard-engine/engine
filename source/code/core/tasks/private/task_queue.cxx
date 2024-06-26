#include <ice/task_queue.hxx>

namespace ice
{

    TaskQueue::TaskQueue(ice::TaskFlags flags) noexcept
        : flags{ flags }
        , _awaitables{ }
    {
    }

    bool TaskQueue::push_back(ice::TaskAwaitableBase* awaitable) noexcept
    {
        ice::linked_queue::push(_awaitables, awaitable);
        _awaitables._head.notify_one();
        return true;
    }

    bool TaskQueue::push_back(ice::LinkedQueueRange<ice::TaskAwaitableBase> awaitable_range) noexcept
    {
        bool const result = ice::linked_queue::push(_awaitables, awaitable_range);
        _awaitables._head.notify_all();
        return result;
    }

    auto TaskQueue::consume() noexcept -> ice::LinkedQueueRange<ice::TaskAwaitableBase>
    {
        return ice::linked_queue::consume(_awaitables);
    }

    bool TaskQueue::process_one(void *result_value) noexcept
    {
        ice::TaskAwaitableBase* const awaitable = ice::linked_queue::pop(_awaitables);
        if (awaitable != nullptr)
        {
            if (result_value != nullptr)
            {
                ICE_ASSERT_CORE(awaitable->result.ptr == nullptr);
                awaitable->result.ptr = result_value;
            }
            awaitable->_coro.resume();
        }
        return awaitable != nullptr;
    }

    auto TaskQueue::process_all(void* result_value) noexcept -> ice::ucount
    {
        ice::ucount processed = 0;
        for (ice::TaskAwaitableBase* const awaitable : ice::linked_queue::consume(_awaitables))
        {
            if (result_value != nullptr)
            {
                ICE_ASSERT_CORE(awaitable->result.ptr == nullptr);
                awaitable->result.ptr = result_value;
            }
            awaitable->_coro.resume();
            processed += 1;
        }
        return processed;
    }

    void TaskQueue::wait_any() noexcept
    {
        _awaitables._head.wait(nullptr, std::memory_order_relaxed);
    }

} // namespace ice
