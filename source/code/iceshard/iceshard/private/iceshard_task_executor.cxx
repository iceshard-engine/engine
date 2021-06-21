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
        , _task_count{ static_cast<ice::u32>(tasks.size()) }
        , _tasks{ ice::move(tasks) }
        , _reset_events_memory{ nullptr }
        , _reset_events{ nullptr }
    {
        if (_task_count > 0)
        {
            // [issue #36] There is currently an issue that prevents to use a std::pmr::vector for
            //  any object containing a std::atomic<T> member.
            //  This issue exists for both the standard vector version and the Iceshard version, `ice::Vector`.
            //  Because of this we allocate enough memory, constructor and pass the needed array of ManualResetEvents manualy.
            _reset_events_memory = _allocator.allocate(_task_count * sizeof(ManualResetEvent) + 8);
            void* aligned_ptr = ice::memory::ptr_align_forward(_reset_events_memory, alignof(ManualResetEvent));

            // Call for each event the default constructor.
            _reset_events = reinterpret_cast<ManualResetEvent*>(aligned_ptr);
            for (ice::u32 idx = 0; idx < _task_count; ++idx)
            {
                new (reinterpret_cast<void*>(_reset_events + idx)) ManualResetEvent{ };
            }
        }
    }

    IceshardTaskExecutor::IceshardTaskExecutor(IceshardTaskExecutor&& other) noexcept
        : _allocator{ other._allocator }
        , _task_count{ other._task_count }
        , _tasks{ ice::move(other._tasks) }
        , _reset_events_memory{ ice::exchange(other._reset_events_memory, nullptr) }
        , _reset_events{ ice::exchange(other._reset_events, nullptr) }
    {
    }

    auto IceshardTaskExecutor::operator=(IceshardTaskExecutor&& other) noexcept -> IceshardTaskExecutor&
    {
        ICE_ASSERT(
            _allocator == other._allocator,
            "Allocator mismatch!"
        );

        _task_count = other._task_count;
        _tasks = ice::move(other._tasks);

        if (_reset_events_memory != nullptr)
        {
            wait_ready();

            // Call for each event their destructor.
            for (ice::u32 idx = 0; idx < _task_count; ++idx)
            {
                (_reset_events + idx)->~ManualResetEvent();
            }

            // Deallocate the memory.
            _allocator.deallocate(_reset_events_memory);
        }

        _reset_events_memory = ice::exchange(other._reset_events_memory, nullptr);
        _reset_events = ice::exchange(other._reset_events, nullptr);
        return *this;
    }

    IceshardTaskExecutor::~IceshardTaskExecutor() noexcept
    {
        if (_reset_events_memory != nullptr)
        {
            wait_ready();

            // Call for each event their destructor.
            for (ice::u32 idx = 0; idx < _task_count; ++idx)
            {
                (_reset_events + idx)->~ManualResetEvent();
            }

            // Deallocate the memory.
            _allocator.deallocate(_reset_events_memory);
        }
    }

    void IceshardTaskExecutor::start_all() noexcept
    {
        for (ice::u32 idx = 0; idx < _task_count; ++idx)
        {
            ice::sync_manual_wait(ice::move(_tasks[idx]), _reset_events[idx]);
            //ice::sync_manual_wait();
            //_tasks[idx] = ice::sync_task(ice::move(_tasks[idx]), _reset_events[idx]);
        }
    }

    void IceshardTaskExecutor::wait_ready() noexcept
    {
        if (_tasks.empty() == false)
        {
            for (ice::u32 idx = 0; idx < _task_count; ++idx)
            {
                _reset_events[idx].wait();
            }

            // [issue #35] Wait for all frame tasks to finish.
            //  Currently the `sync_wait_all` calls the .wait() method on all events.
            //  We should probably re-think the whole API.
            //ice::sync_wait_all(
            //    _allocator,
            //    _tasks,
            //    { _reset_events, _task_count }
            //);

            _tasks.clear();
        }
    }

} // namespace ice
