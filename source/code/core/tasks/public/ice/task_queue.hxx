/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task_types.hxx>
#include <ice/task_awaitable.hxx>
#include <ice/container/linked_queue.hxx>

namespace ice
{

    class TaskQueue final
    {
    public:
        TaskQueue(ice::TaskFlags flags = {}) noexcept;

        bool any() const noexcept { return ice::linked_queue::any(_awaitables); }
        bool empty() const noexcept { return ice::linked_queue::empty(_awaitables); }

        bool push_back(ice::TaskAwaitableBase* awaitable) noexcept;
        bool push_back(ice::LinkedQueueRange<ice::TaskAwaitableBase> awaitable_range) noexcept;

        [[nodiscard]]
        auto consume() noexcept -> ice::LinkedQueueRange<ice::TaskAwaitableBase>;

        bool process_one(void* result_value = nullptr) noexcept;
        auto process_all(void* result_value = nullptr) noexcept -> ice::ucount;

        void wait_any() noexcept;

        template<typename Value>
        inline bool process_one(Value& result_value) noexcept;
        template<typename Value>
        inline auto process_all(Value& result_value) noexcept -> ice::ucount;

        //! \brief Flags of task allowed to be pushed onto this queue.
        ice::TaskFlags const flags;

    private:
        ice::AtomicLinkedQueue<ice::TaskAwaitableBase> _awaitables;
    };

    template<typename Value>
    inline bool TaskQueue::process_one(Value& result_value) noexcept
    {
        return this->process_one(reinterpret_cast<void*>(&result_value));
    }

    template<typename Value>
    inline auto TaskQueue::process_all(Value& result_value) noexcept -> ice::ucount
    {
        return this->process_all(reinterpret_cast<void*>(&result_value));
    }

} // namespace ice
