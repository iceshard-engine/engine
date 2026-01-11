/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

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
        ice::u32 count,
        ice::ShardID shardid
    ) noexcept -> ice::Span<ice::Task<>>
    {
        ice::ncount const new_count = _tasks.size() + count;
        if (new_count > _tasks.capacity())
        {
            // Updates the capacity of the array to fit the new tasks
            _tasks.grow(new_count);
        }

        // Creates the tasks and returns a slice to them
        _tasks.resize(new_count);
        return _tasks.tailspan(_tasks.size() - count);
    }

    auto ScopedTaskContainer::await_tasks_scheduled_on(ice::TaskScheduler& scheduler, ice::TaskScheduler& resumer) noexcept -> ice::Task<>
    {
        co_await ice::await_scheduled_on(_tasks, scheduler, resumer);
    }

    auto ScopedTaskContainer::execute_tasks() noexcept -> ice::u32
    {
        ice::ncount const result = _tasks.size();
        if (result > 0)
        {
            _barrier.reset(result.u8());
            ice::manual_wait_for(_barrier, _tasks);
            _tasks.clear();
        }
        return result.u32();
    }

    auto ScopedTaskContainer::running_tasks() const noexcept -> ice::u32
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
