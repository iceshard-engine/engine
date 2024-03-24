/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task_types.hxx>
#include <ice/task_debug_allocator.hxx>
#include <ice/profiler.hxx>

namespace ice
{

    struct TaskPromiseBase
    {
        struct FinalAwaitable
        {
            constexpr bool await_ready() const noexcept { return false; }

            template<typename Promise>
            inline auto await_suspend(ice::coroutine_handle<Promise> coro) noexcept -> ice::coroutine_handle<>;

            inline void await_resume() const noexcept;
        };

        inline auto initial_suspend() const noexcept;

        inline auto final_suspend() const noexcept;

        inline auto set_continuation(ice::coroutine_handle<> coro) noexcept;

        inline auto continuation() const noexcept -> ice::coroutine_handle<>;

        inline void unhandled_exception() const noexcept;
    protected:
        inline TaskPromiseBase() noexcept = default;

    private:
        ice::coroutine_handle<> _continuation;

    public: // Override to track allocations of task objects
        using TaskDebugAllocator = ice::detail::TaskDebugAllocator;

        inline auto operator new(size_t size) noexcept -> void*
        {
            void* const ptr = TaskDebugAllocator::allocate(size);
            IPT_ALLOC_POOL(ptr, size, TaskDebugAllocator::pool());
            return ptr;
        }

        inline void operator delete(void* ptr) noexcept
        {
            IPT_DEALLOC_POOL(ptr, TaskDebugAllocator::pool());
            TaskDebugAllocator::deallocate(ptr);
        }
    };

    template<typename Promise>
    inline auto TaskPromiseBase::FinalAwaitable::await_suspend(ice::coroutine_handle<Promise> coro) noexcept -> ice::coroutine_handle<>
    {
        ice::coroutine_handle<> continuation = coro.promise().continuation();
        if (continuation == nullptr)
        {
            continuation = std::noop_coroutine();
        }
        return continuation;
    }

    inline void TaskPromiseBase::FinalAwaitable::await_resume() const noexcept
    {
        // Should never be executed
        ICE_ASSERT_CORE(false);
    }

    inline auto TaskPromiseBase::initial_suspend() const noexcept
    {
        return ice::suspend_always{ };
    }

    inline auto TaskPromiseBase::final_suspend() const noexcept
    {
        return FinalAwaitable{ };
    }

    inline auto TaskPromiseBase::set_continuation(ice::coroutine_handle<> coro) noexcept
    {
        _continuation = coro;
    }

    inline auto TaskPromiseBase::continuation() const noexcept -> ice::coroutine_handle<>
    {
        return _continuation;
    }

    inline void TaskPromiseBase::unhandled_exception() const noexcept
    {
        ICE_ASSERT(false, "Unexpected exception in task promise! IceShard does not support exceptions!");
    }

} // namespace ice
