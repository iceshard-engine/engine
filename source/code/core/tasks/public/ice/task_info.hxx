/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task_types.hxx>
#include <atomic>

namespace ice
{

    //! \brief All states a task can be in.
    enum class TaskState : ice::u8
    {
        //! \brief Special state from TaskHandles, handle is not initialized.
        None,
        //! \brief Special state from TaskHandles, handle is not initialized.
        Invalid = None,

        //! \brief Task exists, but execution did not start.
        Created = 0x01,

        //! \brief Task, or one of it's subtasks are executing.
        Running = 0x02,

        //! \brief Task is suspended and awaits resuming. (Unused)
        Suspended = 0x04,

        //! \brief Task finished execution with a valid result.
        Succeeded = 0x08,

        //! \brief Task was canceled at any point of it's lifetime.
        Canceled = 0x10,

        //! \brief Task finished execution but results are invalid.
        Failed = 0x20,

        All = Created | Running | Suspended | Succeeded | Canceled | Failed,
    };

    template<bool IsProfilerEnabled = false>
    struct TaskProfilingInfo { };

    template<>
    struct TaskProfilingInfo<true>
    {
        char const* fiber_name;
    };

    struct TaskInfo final
    {
        static constexpr bool HasProfilingInfo = false;

        TaskInfo() noexcept = default;
        TaskInfo(TaskInfo&&) noexcept = delete;
        TaskInfo(TaskInfo const&) noexcept = delete;
        auto operator=(TaskInfo&&) noexcept -> TaskInfo& = delete;
        auto operator=(TaskInfo const&) noexcept -> TaskInfo& = delete;

        auto aquire() noexcept -> ice::TaskInfo*;
        void release() noexcept;

        bool has_any(ice::TaskState state) const noexcept;

        ice::TaskProfilingInfo<false> profiling;
        std::atomic<ice::TaskState> state = TaskState::Created;

    private:
        std::atomic<ice::u8> _refcount = 1;
    };

    inline auto TaskInfo::aquire() noexcept -> TaskInfo*
    {
        ICE_ASSERT_CORE(_refcount.load(std::memory_order_relaxed) < 255);
        _refcount.fetch_add(1, std::memory_order_relaxed);
        return this;
    }

    inline void TaskInfo::release() noexcept
    {
        ICE_ASSERT_CORE(_refcount.load(std::memory_order_relaxed) != 0);
        if (_refcount.fetch_sub(1, std::memory_order_relaxed) == 0)
        {
            delete this;
        }
    }

    inline bool TaskInfo::has_any(ice::TaskState flags) const noexcept
    {
        return ice::has_any(state.load(std::memory_order_relaxed), flags);
    }

} // namespace ice
