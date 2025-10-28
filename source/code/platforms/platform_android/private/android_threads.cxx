/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "android_threads.hxx"
#include <ice/task_thread.hxx>
#include <ice/log.hxx>
#include <sys/sysinfo.h>

namespace ice::platform::android
{

    AndroidThreads::AndroidThreads(
        ice::Allocator& alloc,
        ice::Span<ice::Shard const> params
    ) noexcept
        : _scheduler_main{ queue_main }
        , _scheduler_gfx{ queue_gfx }
        , _scheduler_tasks{ queue_tasks }
        , _threads{ }
        , _aio_port{ ice::native_aio::aio_open(alloc, ice::native_aio::AIOPortInfo{ .worker_limit = 1, .debug_name = "ice.aio-port" }) }
    {
        ice::ucount const hw_concurrency = get_nprocs();
        ICE_LOG(LogSeverity::Info, LogTag::System, "Logical Processors: {}", hw_concurrency);
        ice::ucount tp_size = ice::max(ice::min(hw_concurrency, 8u), 2u); // min 2 task threads

        for (ice::Shard const option : params)
        {
            if (option == Shard_ThreadPoolSize)
            {
                tp_size = ice::shard_shatter<ice::u32>(option, tp_size);
            }
        }

        // One could force threadpool size to 0 from the params.
        _threads = ice::create_thread_pool(
            alloc, queue_tasks,
            TaskThreadPoolCreateInfo {
                .thread_count = tp_size,
                .debug_name_format = "ice.worker {}",
            }
        );

        _threads->attach_thread(
            "platform.graphics-thread"_sid,
            ice::create_thread(alloc, queue_gfx, { .exclusive_queue = true, .debug_name = "ice.gfx" })
        );
    }

    AndroidThreads::~AndroidThreads() noexcept
    {
        ice::native_aio::aio_close(_aio_port);
    }

} // namespace ice::platform::android
