/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task_types.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/string_types.hxx>

namespace ice
{

    using TaskThreadProcedure = auto(void* userdata, ice::TaskQueue&) noexcept -> ice::u32;

    struct TaskThreadInfo
    {
        //! \brief Consume all tasks from the queue instead of just one from the front.
        //!
        //! \note May yield better results in single-consumer multi-producer scenarios.
        //! \note Executes tasks using FIFO strategy, unless 'sort_by_priority' is set.
        bool exclusive_queue = false;

        //! \brief Enable sorting tasks by priority for this thread if 'exclusive' mode is also set.
        bool sort_by_priority = false;

        //! \brief Custom stack size for the thread.
        //!
        //! \note If the value is '0' it will use the default size.
        ice::usize stack_size = 0_B;

        //! \brief Uses the custom provided procedure to run tasks instead of the built-in implementations.
        //!
        //! \note Note that both 'exclusive_queue' and 'sort_by_priority' are unused in such a case.
        ice::TaskThreadProcedure* custom_procedure;

        //! \brief User-data associated with the custom procedure (if set).
        void* custom_procedure_userdata;

        //! \brief Sets the name of the thread.
        //!
        //! \note May be ignored in some builds.
        ice::String debug_name;
    };

    class TaskThread
    {
    public:
        virtual ~TaskThread() noexcept = default;
        virtual auto info() const noexcept -> ice::TaskThreadInfo const& = 0;
        virtual bool is_busy() const noexcept = 0;
        virtual bool is_running() const noexcept = 0;

        virtual auto estimated_task_count() const noexcept -> ice::ucount = 0;
        //virtual auto queue_pusher() const noexcept -> ice::tasks::v2::TaskQueuePusher & = 0;

        virtual auto queue() noexcept -> ice::TaskQueue & = 0;
    };

    auto create_thread(
        ice::Allocator& alloc,
        ice::TaskQueue& queue,
        ice::TaskThreadInfo const& thread_info
    ) noexcept -> ice::UniquePtr<ice::TaskThread>;

} // namespace ice
