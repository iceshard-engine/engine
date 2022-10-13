#pragma once
#include <ice/container/array.hxx>
#include <coroutine>
#include <atomic>

namespace ice
{

    using TaskList = ice::Array<std::coroutine_handle<>, ice::CollectionLogic::Complex>;

    class ScopedTaskList final
    {
    public:
        ScopedTaskList(
            std::atomic<ice::TaskList*>& owning_pointer,
            ice::TaskList* aquired_list
        ) noexcept;
        ~ScopedTaskList() noexcept;

        ScopedTaskList(ice::ScopedTaskList&&) noexcept = delete;
        auto operator=(ice::ScopedTaskList&&) noexcept -> ice::ScopedTaskList& = delete;

        ScopedTaskList(ice::ScopedTaskList const&) noexcept = delete;
        auto operator=(ice::ScopedTaskList const& other) noexcept -> ice::ScopedTaskList& = delete;

        operator ice::TaskList&() noexcept;
        operator ice::TaskList const&() const noexcept;

    private:
        std::atomic<ice::TaskList*>& _owning_pointer;
        ice::TaskList* _task_list;
    };

    class TaskListContainer final
    {
    public:
        TaskListContainer(ice::Allocator& alloc) noexcept;
        ~TaskListContainer() noexcept;

        auto aquire_list() noexcept -> ice::ScopedTaskList;
        void swap_and_aquire_tasks(
            ice::TaskList& target_list
        ) noexcept;

    private:
        ice::Allocator& _allocator;

        ice::u32 _mutable_task_list_index;
        ice::TaskList* _task_lists[2];
        std::atomic<ice::TaskList*> _mutable_task_list;
    };

} // namespace ice
