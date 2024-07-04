#include <ice/task_scoped_container.hxx>
#include <ice/task_utils.hxx>

namespace ice
{

    ScopedTaskContainer::ScopedTaskContainer(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _tasks{ _allocator }
        , _barrier{ }
    {
    }

    ScopedTaskContainer::~ScopedTaskContainer() noexcept
    {
        // Executes and waits for all tasks on scope exit
        execute_tasks();
        wait_tasks();
    }

    auto ScopedTaskContainer::create_tasks(
        ice::ucount count,
        ice::ShardID shardid
    ) noexcept -> ice::Span<ice::Task<>>
    {
        ice::ucount const new_count = ice::array::count(_tasks) + count;

        if (new_count > ice::array::capacity(_tasks))
        {
            // Updates the capacity of the array to fit the new tasks
            ice::array::grow(_tasks, new_count);
        }

        // Creates the tasks and returns a slice to them
        ice::array::resize(_tasks, new_count);
        return ice::array::slice(_tasks, ice::array::count(_tasks) - count, count);
    }

    auto ScopedTaskContainer::execute_tasks() noexcept -> ice::ucount
    {
        ice::ucount const result = ice::array::count(_tasks);
        if (result > 0)
        {
            _barrier.reset(static_cast<ice::u8>(result));
            ice::manual_wait_for(_barrier, _tasks);
            ice::array::clear(_tasks);
        }
        return result;
    }

    void ScopedTaskContainer::execute_tasks_detached(ice::TaskScheduler& scheduler) noexcept
    {
        ice::execute_tasks(_tasks);
        ice::array::clear(_tasks);
    }

    auto ScopedTaskContainer::running_tasks() const noexcept -> ice::ucount
    {
        return _barrier.value();
    }

    void ScopedTaskContainer::wait_tasks() noexcept
    {
        _barrier.wait();
    }

    auto ScopedTaskContainer::extract_tasks() noexcept -> ice::Array<ice::Task<>>
    {
        return ice::move(_tasks);
    }

} // namespace ice
