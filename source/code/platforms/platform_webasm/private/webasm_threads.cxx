/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "webasm_threads.hxx"
#include <ice/task_thread.hxx>
#include <ice/log.hxx>
#include <emscripten.h>

namespace ice::platform::webasm
{

    // Allows us to access system core count
    EM_JS(int, browser_hardware_concurrency, (), {
        return window.navigator.hardwareConcurrency;
    });

    WebASM_Threads::WebASM_Threads(
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
    {
        ice::ucount const hw_concurrency = browser_hardware_concurrency();
        ICE_LOG(LogSeverity::Info, LogTag::System, "Logical Processors: {}", hw_concurrency);
        ice::ucount tp_size = ice::max(ice::min(hw_concurrency, 4u), 2u); // min 2 task threads

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
    }

} // namespace ice::platform::webasm
