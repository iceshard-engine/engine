#include <ice/task_utils.hxx>
#include <ice/task.hxx>
#include <ice/sync_manual_events.hxx>
#include "internal_task.hxx"

#include <ice/mem_allocator_host.hxx>
#include <ice/container/array.hxx>

namespace ice
{

    namespace detail
    {

        struct DetachedTask
        {
            struct promise_type
            {
                auto initial_suspend() const noexcept { return std::suspend_never{ }; }
                auto final_suspend() const noexcept { return std::suspend_never{ }; }
                auto return_void() noexcept { }

                auto get_return_object() noexcept { return DetachedTask{ }; }
                void unhandled_exception() noexcept
                {
                    ICE_ASSERT(false, "Unexpected coroutine exception!");
                }
            };
        };

        template<typename T>
        concept HasSetMethod = requires(T t) {
            { t.set() } -> std::convertible_to<void>;
        };

        auto detached_task(ice::Task<void> awaited_task, HasSetMethod auto& manual_reset_sem) noexcept -> DetachedTask
        {
            co_await awaited_task;
            manual_reset_sem.set();
        }

    } // namespace detail

    void wait_for(ice::Task<void> task) noexcept
    {
        ice::ManualResetEvent ev;
        manual_wait_for(ice::move(task), ev);
        ev.wait();
    }

    void wait_for_all(ice::Span<ice::Task<void>> tasks) noexcept
    {
        ice::ManualResetSemaphore manual_sem;
        manual_wait_for_all(tasks, manual_sem);
        manual_sem.wait();
    }

    void manual_wait_for(ice::Task<void> task, ice::ManualResetEvent& manual_event) noexcept
    {
        detail::detached_task(ice::move(task), manual_event);
    }

    void manual_wait_for_all(ice::Span<ice::Task<void>> tasks, ice::ManualResetSemaphore& manual_sem) noexcept
    {
        for (ice::Task<void>& task : tasks)
        {
            detail::detached_task(ice::move(task), manual_sem);
        }
    }

} // namespace ice
