/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT


namespace ice
{

    namespace detail
    {

        template<typename T>
        auto output_result_task(ice::Task<T> task, T& out_result) noexcept -> ice::Task<>
        {
            out_result = co_await ice::move(task);
        }

        template<typename T>
        auto output_result_task(ice::TaskExpected<T> task, ice::Expected<T>& out_result) noexcept -> ice::Task<>
        {
            out_result = co_await ice::move(task);
        }

    } // namespace detail

    inline auto resume_on(ice::TaskScheduler& scheduler) noexcept
    {
        return scheduler.operator co_await();
    }

    inline auto await_tasks(ice::Span<ice::Task<>> tasks) noexcept -> ice::Task<>
    {
        for (ice::Task<>& task : tasks)
        {
            co_await task;
        }
    }

    template<typename T>
    inline auto await_on(ice::Task<T> task, ice::TaskScheduler& resumer) noexcept -> ice::Task<T>
    {
        T result;
        co_await ice::await_on(ice::detail::output_result_task(ice::move(task), result), resumer);
        co_return result;
    }

    template<typename T>
    inline auto await_scheduled(ice::Task<T> task, ice::TaskScheduler& scheduler) noexcept -> ice::Task<T>
    {
        T result;
        co_await ice::await_scheduled(ice::detail::output_result_task(ice::move(task), result), scheduler);
        co_return result;
    }

    template<typename T>
    inline auto await_scheduled_on(ice::Task<> task, ice::TaskScheduler& scheduler, ice::TaskScheduler& resumer) noexcept -> ice::Task<T>
    {
        T result;
        co_await ice::await_scheduled_on(ice::detail::output_result_task(ice::move(task), result), scheduler, resumer);
        co_return result;
    }

    template<typename T>
    inline auto wait_for_result(ice::Task<T> task) noexcept -> T
    {
        T result{};
        ice::wait_for(ice::detail::output_result_task(ice::move(task), result));
        return result;
    }

    template<typename T>
    inline void wait_for_result(ice::Task<T> task, T& out_result) noexcept
    {
        ice::wait_for(ice::detail::output_result_task(ice::move(task), out_result));
    }

    template<typename T>
    inline void wait_for_result(ice::Span<ice::Task<T>> tasks, ice::Span<T> out_results) noexcept
    {
        ICE_ASSERT_CORE(ice::count(tasks) == ice::count(out_results));

        // Reset barrier to number of tasks
        ice::ManualResetBarrier barrier{};
        barrier.reset(ice::count(tasks));

        for (ice::u32 idx = 0; idx < ice::count(tasks); ++idx)
        {
            // Wait for each started task manually. So we can await completion after the loop
            ice::manual_wait_for(
                barrier,
                ice::detail::output_result_task(ice::move(tasks[idx]), out_results[idx])
            );
        }

        // Await tasks to finish
        barrier.wait();
    }

    template<typename T>
    inline void wait_for_result_scheduled(ice::Task<T> task, ice::TaskScheduler& scheduler, T& out_result) noexcept
    {
        ice::wait_for_scheduled(ice::detail::output_result_task(ice::move(task), out_result), scheduler);
    }

    template<typename T>
    inline void wait_for_result_scheduled(ice::Span<ice::Task<T>> tasks, ice::TaskScheduler& scheduler, ice::Span<T> out_results) noexcept
    {
        ICE_ASSERT_CORE(ice::count(tasks) == ice::count(out_results));

        // Reset barrier to number of tasks
        ice::ManualResetBarrier barrier{};
        barrier.reset(ice::count(tasks));

        for (ice::u32 idx = 0; idx < ice::count(tasks); ++idx)
        {
            // Wait for each started task manually. So we can await completion after the loop
            ice::manual_wait_for_scheduled(
                barrier,
                ice::detail::output_result_task(ice::move(tasks[idx]), out_results[idx]),
                scheduler
            );
        }

        // Await tasks to finish
        barrier.wait();
    }

    template<typename T>
    inline auto wait_for_expected(ice::TaskExpected<T> task) noexcept -> ice::Expected<T>
    {
        ice::Expected<T> result;
        ice::wait_for(ice::detail::output_result_task(ice::move(task), result));
        return result;
    }

} // namespace ice

