#pragma once
#include <ice/task_container.hxx>
#include <ice/container/array.hxx>
#include <ice/sync_manual_events.hxx>

namespace ice
{

    class ScopedTaskContainer : public TaskContainer
    {
    public:
        ScopedTaskContainer(ice::Allocator& alloc) noexcept;
        ~ScopedTaskContainer() noexcept;

        //! \brief Create a new set of tasks and return the slice containing them.
        auto create_tasks(
            ice::ucount count,
            ice::ShardID shardid
        ) noexcept -> ice::Span<ice::Task<>> override;

        //! \brief Execute all tasks that have been created.
        auto execute_tasks() noexcept -> ice::ucount override;

        //! \brief Returns the number of tasks that are currently running.
        auto running_tasks() const noexcept -> ice::ucount override;

        //! \brief Wait for all tasks to complete.
        void wait_tasks() noexcept override;

    private:
        ice::Allocator& _allocator;
        ice::Array<ice::Task<>> _tasks;
        ice::ManualResetBarrier _barrier;
    };

} // namespace ice
