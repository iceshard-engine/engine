#include <ice/task_thread.hxx>
#include <ice/task_list.hxx>
#include <ice/task.hxx>
#include <ice/os/windows.hxx>
#include <ice/memory/stack_allocator.hxx>
#include <thread>

#include "internal_task.hxx"

namespace ice
{

    namespace detail
    {

        auto create_internal_task(ice::Task<void> task) noexcept -> ice::detail::InternalTask
        {
            co_await task;
            co_return;
        }

    } // namespace detail

    class SimpleTaskThread : public ice::TaskThread
    {
    public:
        using ScheduleOperation = ice::ScheduleOperation<ice::TaskThread>;

        SimpleTaskThread(ice::Allocator& alloc) noexcept
            : _thread{ }
            , _task_lists{ alloc }
        {
            _thread = std::thread{ std::bind(&SimpleTaskThread::thread_routine, this) };
        }

        ~SimpleTaskThread() noexcept override
        {
            join();

            // Destroy any tasks that are still waiting for processing
            ice::memory::StackAllocator_1024 thread_alloc{ };
            ice::TaskList thread_tasks{ thread_alloc };

            ice::pod::array::reserve(thread_tasks, thread_alloc.Constant_BufferSize / sizeof(std::coroutine_handle<>) - 1);
            _task_lists.swap_and_aquire_tasks(thread_tasks);

            ice::detail::ScheduleOperationData* expected_initial_task = _head.load(std::memory_order_acquire);
            if (expected_initial_task != nullptr)
            {
                while (
                    _head.compare_exchange_weak(
                        expected_initial_task,
                        nullptr,
                        std::memory_order::acquire,
                        std::memory_order::relaxed
                    ) == false
                )
                {
                    continue;
                }

                while (expected_initial_task != nullptr)
                {
                    ice::detail::ScheduleOperationData* next_op = expected_initial_task->_next;
                    expected_initial_task->_coroutine.destroy();
                    expected_initial_task = next_op;
                }
            }

            for (std::coroutine_handle<> coro_task : thread_tasks)
            {
                coro_task.destroy();
            }
        }

        void stop() noexcept
        {
            _stop_requested = true;
        }

        void join() noexcept
        {
            if (_thread.joinable())
            {
                _thread.join();
            }
        }

        void schedule_internal(
            ScheduleOperation* op,
            ScheduleOperation::DataMemberType data_member
        ) noexcept override
        {
            ice::detail::ScheduleOperationData& data = op->*data_member;
            ice::detail::ScheduleOperationData* expected_head = _head.load(std::memory_order_acquire);

            do
            {
                data._next = expected_head;
            }
            while (
                _head.compare_exchange_weak(
                    expected_head,
                    &data,
                    std::memory_order_release,
                    std::memory_order_acquire
                ) == false
            );
        }

        void schedule(ice::Task<void> task) noexcept override
        {
            ice::ScopedTaskList scoped_list = _task_lists.aquire_list();
            ice::TaskList& task_list = scoped_list;

            ice::pod::array::push_back(
                task_list,
                detail::create_internal_task(ice::move(task)).move_and_reset()
            );
        }

    protected:
        void thread_routine() noexcept
        {
            ice::memory::StackAllocator_1024 thread_alloc{ };
            ice::TaskList thread_tasks{ thread_alloc };

            ice::pod::array::reserve(thread_tasks, thread_alloc.Constant_BufferSize / sizeof(std::coroutine_handle<>) - 1);

            ice::u32 loops = 0;
            while (_stop_requested == false)
            {
                _task_lists.swap_and_aquire_tasks(thread_tasks);

                ice::detail::ScheduleOperationData* expected_initial_task = _head.load(std::memory_order_acquire);
                if (expected_initial_task != nullptr)
                {
                    while (
                        _head.compare_exchange_weak(
                            expected_initial_task,
                            nullptr,
                            std::memory_order::acquire,
                            std::memory_order::relaxed
                        ) == false
                    )
                    {
                        continue;
                    }
                }

                if (ice::pod::array::empty(thread_tasks) && expected_initial_task == nullptr)
                {
                    SleepEx(1, 0);
                }
                else
                {
                    while (expected_initial_task != nullptr)
                    {
                        ice::detail::ScheduleOperationData* next_op = expected_initial_task->_next;

                        std::coroutine_handle<> coro_task = expected_initial_task->_coroutine;
                        coro_task.resume();
                        //if (; coro_task.done())
                        //{
                        //    //coro_task.destroy();
                        //}
                        //else
                        //{
                        //    ICE_LOG(ice::LogSeverity::Warning, ice::LogTag::Engine, "TThread, task not finished!");
                        //}

                        expected_initial_task = next_op;
                    }

                    for (std::coroutine_handle<> coro_task : thread_tasks)
                    {
                        if (coro_task.resume(); coro_task.done())
                        {
                            coro_task.destroy();
                        }
                        else
                        {
                            ICE_LOG(ice::LogSeverity::Warning, ice::LogTag::Engine, "TThread, task not finished!");
                        }
                    }
                }
            }
        }

    private:
        std::thread _thread;
        bool _stop_requested = false;

        std::atomic<ice::detail::ScheduleOperationData*> _head = nullptr;
        ice::TaskListContainer _task_lists;
    };

    auto create_task_thread(ice::Allocator& alloc) noexcept -> ice::UniquePtr<TaskThread>
    {
        return ice::make_unique<TaskThread, SimpleTaskThread>(alloc, alloc);
    }

} // namespace ice
