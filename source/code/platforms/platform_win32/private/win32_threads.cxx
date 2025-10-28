/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "win32_threads.hxx"
#include <ice/task_thread.hxx>
#include <ice/os.hxx>
#include <bit>

namespace ice::platform::win32
{

    auto get_num_cores(ice::Allocator& alloc) noexcept -> ice::ucount
    {
        DWORD byte_size = 0;
        GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &byte_size);
        ICE_ASSERT_CORE(GetLastError() == ERROR_INSUFFICIENT_BUFFER);

        ice::AllocResult const buffer = alloc.allocate(ice::usize{ byte_size });
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* info =
            (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*) buffer.memory;

        DWORD const result = GetLogicalProcessorInformationEx(
            RelationProcessorCore, info, &byte_size
        );
        ICE_ASSERT_CORE(result == TRUE);

        ice::ucount num_procs = 0;
        ice::usize byte_size_processed = 0_B;
        for (; byte_size_processed.value < byte_size;)
        {
            // TODO: Return core counts for different efficiency classes.
            // info->Processor.EfficiencyClass

            // If we have more than one logical processor in this core.
            if ((info->Processor.Flags & LTP_PC_SMT))
            {
                for (GROUP_AFFINITY const& group : ice::Span{ info->Processor.GroupMask, info->Processor.GroupCount })
                {
                    num_procs += std::popcount(group.Mask);
                }
            }
            else
            {
                num_procs += 1;
            }

            // Move to the next structure.
            byte_size_processed += { info->Size };
            info = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*) ice::ptr_add(
                buffer.memory, byte_size_processed
            );
        }

        alloc.deallocate(buffer);
        return num_procs;
    }

    Win32Threads::Win32Threads(
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
        ice::ucount const hw_concurrency = ice::min(get_num_cores(alloc), 8u);
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
                .debug_name_format = "ice.worker {}",
            }
        );

        // Attache the graphics thread to the threadpool so we don't need to manage it ourselfs.
        _threads->attach_thread("platform.graphics-thread"_sid, ice::move(gfx_thread));
    }

    Win32Threads::~Win32Threads() noexcept
    {
        // We close the port first (threads implicitly) to ensure we don't get stuck on io-port waiting indefinitely.
        ice::native_aio::aio_close(_aioport);
    }

} // namespace ice::platform::win32
