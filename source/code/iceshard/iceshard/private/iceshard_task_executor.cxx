/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/task_utils.hxx>
#include "iceshard_task_executor.hxx"
#include <ice/sync_manual_events.hxx>
#include <ice/container/array.hxx>

namespace ice
{

    IceshardTaskExecutor::IceshardTaskExecutor(
        ice::Allocator& alloc,
        IceshardTaskExecutor::TaskList tasks
    ) noexcept
        : _allocator{ alloc }
        , _task_count{ tasks.size().u32() }
        , _tasks{ ice::move(tasks) }
        , _sync_sem{ 0 }
    {
        ICE_ASSERT(_task_count < ice::u8_max, "Maximum number of tasks: {}", ice::u8_max);
    }

    IceshardTaskExecutor::IceshardTaskExecutor(IceshardTaskExecutor&& other) noexcept
        : _allocator{ other._allocator }
        , _task_count{ other._task_count }
        , _tasks{ ice::move(other._tasks) }
    {
    }

    auto IceshardTaskExecutor::operator=(IceshardTaskExecutor&& other) noexcept -> IceshardTaskExecutor&
    {
        if (this != &other)
        {
            wait_ready();

            _task_count = other._task_count;
            _tasks = ice::move(other._tasks);
        }
        return *this;
    }

    IceshardTaskExecutor::~IceshardTaskExecutor() noexcept
    {
        wait_ready();
    }

    void IceshardTaskExecutor::start_all() noexcept
    {
        ICE_ASSERT(_sync_sem.is_set(), "Invalid semaphore state!");

        _sync_sem.reset(ice::u8(_task_count));
        ice::manual_wait_for(_sync_sem, _tasks);
    }

    void IceshardTaskExecutor::wait_ready() noexcept
    {
        if (_tasks.not_empty())
        {
            _sync_sem.wait();
            _tasks.clear();
        }
    }

} // namespace ice
