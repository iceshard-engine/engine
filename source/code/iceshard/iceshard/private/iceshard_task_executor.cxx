#include "iceshard_task_executor.hxx"
#include <ice/task_sync_wait.hxx>
#include <ice/sync_manual_events.hxx>
#include <ice/container/array.hxx>

namespace ice
{

    IceshardTaskExecutor::IceshardTaskExecutor(
        ice::Allocator& alloc,
        IceshardTaskExecutor::TaskList tasks
    ) noexcept
        : _allocator{ alloc }
        , _task_count{ ice::count(tasks) }
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
            _reset_events_memory = _allocator.allocate(ice::meminfo_of<ManualResetEvent> * _task_count);

            // Call for each event the default constructor.
            _reset_events = reinterpret_cast<ManualResetEvent*>(_reset_events_memory.location);
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
        , _reset_events_memory{ ice::exchange(other._reset_events_memory, { }) }
        , _reset_events{ ice::exchange(other._reset_events, nullptr) }
    {
    }

    auto IceshardTaskExecutor::operator=(IceshardTaskExecutor&& other) noexcept -> IceshardTaskExecutor&
    {
        ICE_ASSERT(
            &_allocator == &other._allocator,
            "Allocator mismatch!"
        );

        _task_count = other._task_count;
        _tasks = ice::move(other._tasks);

        if (_reset_events_memory.location != nullptr)
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

        _reset_events_memory = ice::exchange(other._reset_events_memory, { });
        _reset_events = ice::exchange(other._reset_events, nullptr);
        return *this;
    }

    IceshardTaskExecutor::~IceshardTaskExecutor() noexcept
    {
        if (_reset_events_memory.location != nullptr)
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
        if (ice::array::any(_tasks))
        {
            for (ice::u32 idx = 0; idx < _task_count; ++idx)
            {
                _reset_events[idx].wait();
            }

            ice::array::clear(_tasks);
        }
    }

} // namespace ice
