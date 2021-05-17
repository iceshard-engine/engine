#include <ice/task_thread.hxx>
#include <ice/task_scheduler.hxx>
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

        void task_thread_routine(void* task_thread_object) noexcept;

        auto create_internal_task(ice::Task<void> task) noexcept -> ice::detail::InternalTask
        {
            co_await task;
            co_return;
        }

    } // namespace detail

    class SimpleTaskThread : public ice::TaskThread, public ice::TaskScheduler
    {
    public:
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

            ice::ScheduleOperation* expected_initial_task = _head.load(std::memory_order_acquire);
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
                    ScheduleOperation* next_op = expected_initial_task->_next;
                    expected_initial_task->_coro.destroy();
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

        auto scheduler() noexcept -> ice::TaskScheduler&
        {
            return *this;
        }

        void schedule(ice::Task<void> task) noexcept
        {
            ice::ScopedTaskList scoped_list = _task_lists.aquire_list();
            ice::TaskList& task_list = scoped_list;

            ice::pod::array::push_back(
                task_list,
                detail::create_internal_task(ice::move(task)).move_and_reset()
            );
        }

        void schedule_test_internal(ice::ScheduleOperation* schedule_op) noexcept
        {
            ScheduleOperation* expected_head = _head.load(std::memory_order_acquire);

            do
            {
                schedule_op->_next = expected_head;
            }
            while (
                _head.compare_exchange_weak(
                    expected_head,
                    schedule_op,
                    std::memory_order_release,
                    std::memory_order_acquire
                ) == false
            );

        }

        auto schedule_test() noexcept -> ice::ScheduleOperation override
        {
            return ScheduleOperation{ *this };
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

                ice::ScheduleOperation* expected_initial_task = _head.load(std::memory_order_acquire);
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
                        ScheduleOperation* next_op = expected_initial_task->_next;

                        std::coroutine_handle<> coro_task = expected_initial_task->_coro;
                        if (coro_task.resume(); coro_task.done())
                        {
                            coro_task.destroy();
                        }
                        else
                        {
                            ICE_LOG(ice::LogSeverity::Warning, ice::LogTag::Engine, "TThread, task not finished!");
                        }

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

        std::atomic<ice::ScheduleOperation*> _head = nullptr;
        ice::TaskListContainer _task_lists;
    };

    auto create_task_thread(ice::Allocator& alloc) noexcept -> ice::UniquePtr<TaskThread>
    {
        return ice::make_unique<TaskThread, SimpleTaskThread>(alloc, alloc);
    }

    //void detail::task_thread_routine(void* task_thread_object) noexcept
    //{
    //    SimpleTaskThread* task_thread = reinterpret_cast<SimpleTaskThread*>(task_thread_object);

    //}

    void ScheduleOperation::await_suspend(std::coroutine_handle<void> coro) noexcept
    {
        SimpleTaskThread& thread_scheduler = static_cast<SimpleTaskThread&>(_scheduler);

        thread_scheduler.schedule_test_internal(this);
    }

} // namespace ice
