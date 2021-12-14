#include <ice/task_list.hxx>
#include <ice/assert.hxx>
#include <atomic>

namespace ice
{

    ScopedTaskList::ScopedTaskList(
        std::atomic<ice::TaskList*>& owning_pointer,
        ice::TaskList* aquired_list
    ) noexcept
        : _owning_pointer{ owning_pointer }
        , _task_list{ aquired_list }
    {
    }

    ScopedTaskList::~ScopedTaskList() noexcept
    {
        if (_task_list != nullptr)
        {
            ice::TaskList* expected_list = nullptr;

            // We should be the only ones having a task list from this atomic pointer
            //  So we can just strongly require for this operation to succed.
            //  If something fails then there was a corrupting agent involved.
            bool const returned_task_list_successful = _owning_pointer.compare_exchange_strong(
                expected_list,
                _task_list,
                std::memory_order::release
            );

            ICE_ASSERT(
                returned_task_list_successful,
                "Failed to return the scoped task list successful! An unexpected value [{}] was found in the owning pointer reference!",
                fmt::ptr(expected_list)
            );
        }
    }

    ScopedTaskList::operator ice::TaskList&() noexcept
    {
        return *_task_list;
    }

    ScopedTaskList::operator ice::TaskList const&() const noexcept
    {
        return *_task_list;
    }

    TaskListContainer::TaskListContainer(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _mutable_task_list_index{ 0 }
        , _task_lists{ nullptr, nullptr }
        , _mutable_task_list{ nullptr }
    {
        _task_lists[0] = _allocator.make<ice::TaskList>(_allocator);
        _task_lists[1] = _allocator.make<ice::TaskList>(_allocator);
        _mutable_task_list = _task_lists[0];
    }

    TaskListContainer::~TaskListContainer() noexcept
    {
        _allocator.destroy(_task_lists[0]);
        _allocator.destroy(_task_lists[1]);
    }

    auto TaskListContainer::aquire_list() noexcept -> ice::ScopedTaskList
    {
        ice::TaskList* expected_list = _task_lists[_mutable_task_list_index];

        // Try to aquire the pointer to the currently mutable task list
        //  Leave behind a nullptr value, so neither the swap not another aquire will pass their checks.
        while (
            _mutable_task_list.compare_exchange_weak(
                expected_list,
                nullptr,
                std::memory_order::relaxed,
                std::memory_order::acquire
            ) == false
        )
        {
            expected_list = _task_lists[_mutable_task_list_index];
        }

        return ScopedTaskList{ _mutable_task_list, expected_list };
    }

    void TaskListContainer::swap_and_aquire_tasks(
        ice::TaskList& target_list
    ) noexcept
    {
        // Cannot update the `_mutable_task_list_index` as it might be used in the `aquire_list` method.
        ice::u32 const current_mutable_task_list_index = _mutable_task_list_index;
        ice::u32 const new_mutable_task_list_index = (_mutable_task_list_index + 1) % 2;

        ice::TaskList* const current_mutable_list = _task_lists[current_mutable_task_list_index];
        ice::TaskList* const new_mutable_list = _task_lists[new_mutable_task_list_index];

        // Try to replace the current task list with the next one in order.
        //  If the replacement succeeds we have now explicit access to the `current_mutable_list`
        ice::TaskList* expected_list = current_mutable_list;
        while (
            _mutable_task_list.compare_exchange_weak(
                expected_list,
                new_mutable_list,
                std::memory_order::acq_rel,
                std::memory_order::acquire
            ) == false
        )
        {
            expected_list = current_mutable_list;
        }

        // Update the index, else `aquire_list` will never succeed as the `expected_list` pointer
        //  will be different than the one stored in `_mutable_task_list`
        _mutable_task_list_index = new_mutable_task_list_index;

        // Copy all task to the target list
        target_list = *expected_list;
        // Clear the current "read-only" list and finish profit.
        ice::pod::array::clear(*expected_list);
    }

} // namespace ice
