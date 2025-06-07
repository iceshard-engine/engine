/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/platform_threads.hxx>
#include <ice/task_queue.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/task_thread_pool.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/span.hxx>

namespace ice::platform::linux
{

    class LinuxThreads final : public ice::platform::Threads
    {
    public:
        LinuxThreads(
            ice::Allocator& alloc,
            ice::Span<ice::Shard const> params
        ) noexcept;
        ~LinuxThreads() noexcept override;

        auto main() noexcept -> ice::TaskScheduler& override { return _scheduler_main; }
        auto graphics() noexcept -> ice::TaskScheduler& override { return _scheduler_gfx; }

        auto threadpool() noexcept -> ice::TaskScheduler& override { return _scheduler_tasks; }
        auto threadpool_size() const noexcept -> ice::u32 override { return _threads->managed_thread_count(); }
        auto threadpool_object() noexcept -> ice::TaskThreadPool* override { return _threads.get(); }
        auto aio_port() const noexcept -> ice::native_aio::AIOPort override { return _aioport; }

        ice::TaskQueue queue_main;
        ice::TaskQueue queue_gfx;
        ice::TaskQueue queue_tasks;

    private:
        ice::TaskScheduler _scheduler_main;
        ice::TaskScheduler _scheduler_gfx;
        ice::TaskScheduler _scheduler_tasks;
        ice::UniquePtr<ice::TaskThreadPool> _threads;

        ice::native_aio::AIOPort _aioport;
    };

} // namespace ice::platform::linux
