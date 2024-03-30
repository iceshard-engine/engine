#include "win32_threads.hxx"
#include <ice/task_thread.hxx>
#include <ice/os.hxx>

namespace ice::platform::win32
{

    // TODO: Because WebApp only report logical cores this should also do so
    auto get_num_cores(ice::Allocator& alloc) noexcept -> ice::ucount
    {
        DWORD lp_count = 0;
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* lp_info = nullptr;

        GetLogicalProcessorInformationEx(
            LOGICAL_PROCESSOR_RELATIONSHIP::RelationProcessorCore,
            lp_info, &lp_count
        );
        ICE_ASSERT_CORE(GetLastError() == ERROR_INSUFFICIENT_BUFFER);

        lp_info = alloc.allocate<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(lp_count);
        DWORD const result = GetLogicalProcessorInformationEx(
            LOGICAL_PROCESSOR_RELATIONSHIP::RelationProcessorCore,
            lp_info, &lp_count
        );
        ICE_ASSERT_CORE(result == TRUE);

        ice::ucount num_procs = 0;
        for (DWORD idx = 0; idx < lp_count; ++idx)
        {
            switch (lp_info[idx].Relationship)
            {
            case RelationProcessorCore:
                num_procs++;

                // A hyperthreaded core supplies more than one logical processor.
                // logicalProcessorCount += CountSetBits(ptr->ProcessorMask);
                break;
            }
        }

        alloc.deallocate(lp_info);
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
    {
        ice::ucount const hw_concurrency = get_num_cores(alloc);
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
                .debug_name_format = "ice.thread {}",
            }
        );

        // Attache the graphics thread to the threadpool so we don't need to manage it ourselfs.
        _threads->attach_thread("platform.graphics-thread"_sid, ice::move(gfx_thread));
    }

} // namespace ice::platform::win32
