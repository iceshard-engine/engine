
namespace ice::v2
{

    inline auto resume_on(ice::TaskScheduler& scheduler) noexcept
    {
        return scheduler.operator co_await();
    }

    inline auto await_all(ice::Span<ice::Task<>> tasks) noexcept -> ice::Task<>
    {
        for (ice::Task<>& task : tasks)
        {
            co_await task;
        }
    }

#if 0
    inline auto process_all(ice::Span<ice::Task<>> tasks) noexcept -> ice::Task<>
    {
        for (ice::Task<>& task : tasks)
        {
            co_await task;
        }
    }

    inline auto process_queue(ice::TaskQueue& queue, void* result) noexcept -> ice::Task<>
    {
        queue.process_all(result);
        co_return;
    }
#endif

} // namespace ice::v2
