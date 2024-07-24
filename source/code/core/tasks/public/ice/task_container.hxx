#pragma once
#include <ice/task.hxx>
#include <ice/shard.hxx>
#include <ice/span.hxx>
#include <ice/container/array.hxx>

namespace ice
{

    struct TaskContainer
    {
        virtual ~TaskContainer() noexcept = default;

        virtual auto create_tasks(ice::u32 count, ice::ShardID id) noexcept -> ice::Span<ice::Task<>> = 0;

        virtual auto await_tasks_scheduled_on(ice::TaskScheduler& scheduler, ice::TaskScheduler& resumer) noexcept -> ice::Task<> = 0;

        virtual auto execute_tasks() noexcept -> ice::ucount = 0;

        virtual auto running_tasks() const noexcept -> ice::ucount = 0;

        virtual void wait_tasks() noexcept = 0;

        virtual auto extract_tasks() noexcept -> ice::Array<ice::Task<>> = 0;
    };

} // namespace ice
