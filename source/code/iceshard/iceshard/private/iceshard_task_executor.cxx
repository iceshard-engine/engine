#include "iceshard_task_executor.hxx"
#include <ice/memory/pointer_arithmetic.hxx>
#include <ice/task_sync_wait.hxx>
#include <ice/sync_manual_events.hxx>

namespace ice
{

    IceshardTaskExecutor::IceshardTaskExecutor(
        ice::Allocator& alloc,
        ice::Vector<ice::Task<void>> tasks
    ) noexcept
        : _allocator{ alloc }
        , _tasks{ ice::move(tasks) }
    {
    }

    void IceshardTaskExecutor::wait_ready() noexcept
    {
        if (_tasks.empty() == false)
        {
            ice::u32 const task_count = static_cast<ice::u32>(_tasks.size());

            // [issue #36] There is currently an issue that prevents to use a std::pmr::vector for
            //  any object containing a std::atomic<T> member.
            //  This issue exists for both the standard vector version and the Iceshard version, `ice::Vector`.
            //  Because of this we allocate enough memory, constructor and pass the needed array of ManualResetEvents manualy.
            void* ptr = _allocator.allocate(task_count * sizeof(ManualResetEvent) + 8);
            void* aligned_ptr = ice::memory::ptr_align_forward(ptr, alignof(ManualResetEvent));

            // Call for each event the default constructor.
            ManualResetEvent* ptr_events = reinterpret_cast<ManualResetEvent*>(aligned_ptr);
            for (ice::u32 idx = 0; idx < task_count; ++idx)
            {
                new (reinterpret_cast<void*>(ptr_events + idx)) ManualResetEvent{ };
            }

            // [issue #35] Wait for all frame tasks to finish.
            //  Currently the `sync_wait_all` calls the .wait() method on all events.
            //  We should probably re-think the whole API.
            ice::sync_wait_all(
                _allocator,
                _tasks,
                { ptr_events, task_count }
            );

            _tasks.clear();

            // Call for each event their destructor.
            for (ice::u32 idx = 0; idx < task_count; ++idx)
            {
                (ptr_events + idx)->~ManualResetEvent();
            }

            // Deallocate the memory.
            _allocator.deallocate(ptr);
        }
    }

} // namespace ice
