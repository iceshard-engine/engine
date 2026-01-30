/// Copyright 2023 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task_types.hxx>
#include <ice/task_awaitable.hxx>
#include <ice/atomic_linked_queue.hxx>

namespace ice
{

    class TaskQueue final
    {
    public:
        TaskQueue(ice::TaskFlags flags = {}) noexcept;

        bool is_empty() const noexcept { return _awaitables.is_empty(); }
        bool not_empty() const noexcept { return _awaitables.not_empty(); }

        bool push_back(ice::TaskAwaitableBase* awaitable) noexcept;
        bool push_back(ice::AtomicLinkedQueueRange<ice::TaskAwaitableBase> awaitable_range) noexcept;

        bool contains(ice::TaskAwaitableBase* awaitable) const noexcept;

        [[nodiscard]]
        auto take_front() noexcept -> ice::TaskAwaitableBase*;
        [[nodiscard]]
        auto take_all() noexcept -> ice::AtomicLinkedQueueRange<ice::TaskAwaitableBase>;

        bool process_one(void* result_value = nullptr) noexcept;
        auto process_all(void* result_value = nullptr) noexcept -> ice::ncount;

        void wait_any() noexcept;

        template<typename Value>
        constexpr bool process_one(Value& result_value) noexcept;
        template<typename Value>
        constexpr auto process_all(Value& result_value) noexcept -> ice::ncount;

        //! \brief Flags of task allowed to be pushed onto this queue.
        ice::TaskFlags const flags;

    private:
        ice::AtomicLinkedQueue<ice::TaskAwaitableBase> _awaitables;
    };

    template<typename Value>
    inline constexpr bool TaskQueue::process_one(Value& result_value) noexcept
    {
        return this->process_one(reinterpret_cast<void*>(&result_value));
    }

    template<typename Value>
    inline constexpr auto TaskQueue::process_all(Value& result_value) noexcept -> ice::ncount
    {
        return this->process_all(reinterpret_cast<void*>(&result_value));
    }

} // namespace ice
