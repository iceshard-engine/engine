#pragma once
#include <ice/task_types_v3.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/string_types.hxx>

namespace ice
{

    struct TaskThreadInfo
    {
        //! \brief Consume all tasks from the queue instead of just one from the front.
        //! \note May yield better results in single-consumer multi-producer scenarios.
        //! \note Executes tasks using FIFO strategy, unless 'sort_by_priority' is set.
        bool exclusive_queue = false;

        //! \brief Enable sorting tasks by priority for this thread if 'exclusive' mode is also set.
        bool sort_by_priority = false;

        //! \brief Custom stack size for the thread.
        //! \note If the value is '0' it will use the default size.
        ice::usize stack_size = 0_B;

        //! \brief Sets the name of the thread.
        //! \note May be ignored in some builds.
        ice::String debug_name;
    };

    class TaskThread_v3
    {
    public:
        virtual ~TaskThread_v3() noexcept = default;
        virtual auto info() const noexcept -> ice::TaskThreadInfo const& = 0;
        virtual bool is_busy() const noexcept = 0;
        virtual bool is_running() const noexcept = 0;

        virtual auto estimated_task_count() const noexcept -> ice::ucount = 0;
        //virtual auto queue_pusher() const noexcept -> ice::tasks::v2::TaskQueuePusher & = 0;

        virtual auto queue() noexcept -> ice::TaskQueue_v3 & = 0;
    };

    auto create_thread(
        ice::Allocator& alloc,
        ice::TaskQueue_v3& queue,
        ice::TaskThreadInfo const& thread_info
    ) noexcept -> ice::UniquePtr<ice::TaskThread_v3>;

} // namespace ice
