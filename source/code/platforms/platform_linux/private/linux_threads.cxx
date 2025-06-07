/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "linux_threads.hxx"
#include <ice/task_thread.hxx>
#include <ice/log.hxx>
#include <ice/os.hxx>
#include <bit>

namespace ice::platform::linux
{

    auto get_num_cores(ice::Allocator& alloc) noexcept -> ice::ucount
    {
        ice::ucount const hw_concurrency = sysconf(_SC_NPROCESSORS_ONLN);
        ICE_LOG(LogSeverity::Info, LogTag::System, "Logical Processors: {}", hw_concurrency);
        return hw_concurrency;
    }

    LinuxThreads::LinuxThreads(
        ice::Allocator& alloc,
        ice::Span<ice::Shard const> params
    ) noexcept
        : queue_main{ }
        , queue_gfx{ }
        , queue_tasks{ }
        , _scheduler_main{ queue_main }
        , _scheduler_gfx{ queue_gfx }
        , _scheduler_tasks{ queue_tasks }
        , _threads{ }
        , _aioport{ ice::native_aio::aio_open(alloc, { .worker_limit = 2, .debug_name = "ice.aio-port" }) }
    {
        ice::ucount const hw_concurrency = ice::min(get_num_cores(alloc), 8u); // max 8 tasks threads
        ice::ucount tp_size = ice::max(hw_concurrency, 2u); // min 2 task threads

        for (ice::Shard const option : params)
        {
            if (option == Shard_ThreadPoolSize)
            {
                tp_size = ice::shard_shatter<ice::u32>(option, tp_size);
            }
        }

        ice::UniquePtr<ice::TaskThread> gfx_thread = ice::create_thread(
            alloc, queue_gfx,
            TaskThreadInfo{
                .exclusive_queue = true,
                .debug_name = "ice.gfx"
            }
        );

        // One could force threadpool size to 0 from the params.
        _threads = ice::create_thread_pool(
            alloc, queue_tasks,
            TaskThreadPoolCreateInfo {
                .thread_count = tp_size,
                .aioport = _aioport,
                .debug_name_format = "ice.thread {}",
            }
        );

        // Attache the graphics thread to the threadpool so we don't need to manage it ourselfs.
        _threads->attach_thread("platform.graphics-thread"_sid, ice::move(gfx_thread));
    }

    LinuxThreads::~LinuxThreads() noexcept
    {
        // We close the port first (threads implicitly) to ensure we don't get stuck on io-port waiting indefinitely.
        ice::native_aio::aio_close(_aioport);
    }

} // namespace ice::platform::win32
