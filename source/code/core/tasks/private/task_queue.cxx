/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

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
        _awaitables.push_back(awaitable);
        _awaitables._head.notify_one();
        return true;
    }

    bool TaskQueue::push_back(ice::AtomicLinkedQueueRange<ice::TaskAwaitableBase> awaitable_range) noexcept
    {
        bool const result = _awaitables.push_back(awaitable_range);
        _awaitables._head.notify_all();
        return result;
    }

    bool TaskQueue::contains(ice::TaskAwaitableBase* awaitable) const noexcept
    {
        auto* volatile it = _awaitables._head.load(std::memory_order_relaxed);
        if (it != nullptr)
        {
            auto* const end = _awaitables._tail.load(std::memory_order_relaxed);

            // Loop when multiple awaitables where pushed after setting the test.
            while(it != end && it != awaitable)
            {
                // We wait for next pointer to be updated
                while(it->_next == nullptr)
                {
                    std::atomic_thread_fence(std::memory_order_acquire);
                }

                it = it->_next;
            }

            // Found our awaitable don't suspend
            if (it == awaitable)
            {
                return false;
            }
        }
        return true;
    }

    auto TaskQueue::take_all() noexcept -> ice::AtomicLinkedQueueRange<ice::TaskAwaitableBase>
    {
        return _awaitables.take_all();
    }

    auto TaskQueue::take_front() noexcept -> ice::TaskAwaitableBase*
    {
        return _awaitables.take_front();
    }

    bool TaskQueue::process_one(void* result_value) noexcept
    {
        ice::TaskAwaitableBase* const awaitable = _awaitables.take_front();
        if (awaitable != nullptr)
        {
            if (result_value != nullptr)
            {
                ICE_ASSERT_CORE(awaitable->result.ptr == nullptr);
                awaitable->result.ptr = result_value;
            }

            // Support custom resumer logic
            if (awaitable->_params.modifier == ice::TaskAwaitableModifier::CustomResumer)
            {
                void* resumer_ptr = awaitable->result.ptr;
                ICE_ASSERT_CORE(resumer_ptr != nullptr && result_value == nullptr);

                ice::TaskAwaitableCustomResumer const* custom_resumer = reinterpret_cast<ice::TaskAwaitableCustomResumer*>(resumer_ptr);
                if (custom_resumer->fn_resumer(custom_resumer->ud_resumer, *awaitable) == false)
                {
                    // Reset next pointer before puttng back onto the queue
                    awaitable->_next = nullptr;

                    // Push back at the end of the queue
                    _awaitables.push_back(awaitable);
                    return false;
                }
            }

            awaitable->_coro.resume();
        }
        return awaitable != nullptr;
    }

    auto TaskQueue::process_all(void* result_value) noexcept -> ice::ncount
    {
        ice::u32 processed = 0;
        for (ice::TaskAwaitableBase* const awaitable : _awaitables.take_all())
        {
            if (result_value != nullptr)
            {
                ICE_ASSERT_CORE(awaitable->result.ptr == nullptr);
                awaitable->result.ptr = result_value;
            }

            // Support custom resumer logic
            if (awaitable->_params.modifier == ice::TaskAwaitableModifier::CustomResumer)
            {
                void* resumer_ptr = awaitable->result.ptr;
                ICE_ASSERT_CORE(resumer_ptr != nullptr && result_value == nullptr);

                ice::TaskAwaitableCustomResumer const* custom_resumer = reinterpret_cast<ice::TaskAwaitableCustomResumer*>(resumer_ptr);
                if (custom_resumer->fn_resumer(custom_resumer->ud_resumer, *awaitable) == false)
                {
                    // Reset next pointer before puttng back onto the queue
                    awaitable->_next = nullptr;

                    // Push back at the end of the queue
                    _awaitables.push_back(awaitable);
                    continue;
                }
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
