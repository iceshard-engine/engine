#include <ice/sync_manual_events.hxx>
#include <ice/task.hxx>
#include <ice/task_sync_wait.hxx>

#include <ice/collections.hxx>
#include <ice/memory/memory_globals.hxx>
#include <ice/assert.hxx>

#include "internal_task.hxx"

namespace ice
{

    namespace detail
    {

        struct OneWaytask
        {
            struct promise_type
            {
                auto initial_suspend() const noexcept { return std::suspend_never{ }; }
                auto final_suspend() const noexcept { return std::suspend_never{ }; }
                auto return_void() noexcept { }

                auto get_return_object() noexcept { return OneWaytask{ }; }
                void unhandled_exception() noexcept
                {
                    ICE_ASSERT(false, "Unexpected coroutine exception!");
                }
            };
        };

        auto create_internal_synced_task(
            ice::Task<void> task,
            ice::ManualResetEvent* reset_event
        ) noexcept -> ice::detail::InternalTask
        {
            co_await task;
            reset_event->set();

            co_return;
        }

    } // namespace detail

    void sync_wait(
        ice::Task<void> task
    ) noexcept
    {
        ManualResetEvent sync_event{ };

        ice::detail::InternalTask internal_task = detail::create_internal_synced_task(
            ice::move(task),
            &sync_event
        );

        internal_task.resume();
        sync_event.wait();
    }

    void sync_manual_wait(
        ice::Task<void> task,
        ice::ManualResetEvent& reset_event
    ) noexcept
    {
        [](ice::Task<void> task, ice::ManualResetEvent* reset_event) noexcept -> detail::OneWaytask
        {
            co_await task;
            int x = 3;
            reset_event->set();
            co_return;

        }(ice::move(task), &reset_event);
    }

    void sync_wait_all(
        ice::Allocator& alloc,
        ice::Span<ice::Task<void>> tasks,
        ice::Span<ice::ManualResetEvent> reset_events
    ) noexcept
    {
        ice::u32 const task_count = ice::size(tasks);

        ICE_ASSERT(
            task_count == ice::size(reset_events),
            "Provided number of reset events does not match the number of tasks!"
        );

        ice::Vector<ice::detail::InternalTask> internal_tasks{ alloc };
        internal_tasks.reserve(task_count);

        for (ice::u32 idx = 0; idx < task_count; ++idx)
        {
            ice::detail::InternalTask internal_task = detail::create_internal_synced_task(
                ice::move(tasks[idx]),
                &reset_events[idx]
            );

            internal_task.resume();
            internal_tasks.push_back(ice::move(internal_task));
        }

        for (ice::u32 idx = 0; idx < task_count; ++idx)
        {
            reset_events[idx].wait();
        }

        for (ice::detail::InternalTask& task : internal_tasks)
        {
            if (task.is_ready() == false)
            {
                ICE_LOG(ice::LogSeverity::Warning, ice::LogTag::Engine, "Task not finished!");
            }
        }
    }

    //void when_all_ready(
    //    //ice::Allocator& alloc,
    //    ice::Span<ice::Task<void>> tasks,
    //    ice::Span<ice::ManualResetEvent> reset_events
    //) noexcept
    //{
    //    []() noexcept -> detail::OneWayTask
    //    {

    //    }();
    //}

    auto sync_task(ice::Task<void> task, ice::ManualResetEvent* reset_event) noexcept -> ice::Task<>
    {
        ice::detail::InternalTask internal_task = detail::create_internal_synced_task(
            ice::move(task),
            reset_event
        );

        internal_task.resume();
        reset_event->wait();
        co_return;
    }

} // namespace ice
