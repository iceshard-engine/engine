#pragma once
#include <ice/task.hxx>
#include <ice/shard.hxx>
#include <ice/span.hxx>

namespace ice
{

    struct TaskContainer
    {
        virtual ~TaskContainer() noexcept = default;

        virtual auto create_tasks(ice::u32 count, ice::ShardID id) noexcept -> ice::Span<ice::Task<>> = 0;

        virtual auto execute_tasks() noexcept -> ice::ucount = 0;

        // TODO: Remove? Should be a temporary solution
        virtual void execute_tasks_detached(ice::TaskScheduler& scheduler) noexcept { }

        virtual auto running_tasks() const noexcept -> ice::ucount = 0;

        virtual void wait_tasks() noexcept = 0;
    };

} // namespace ice
