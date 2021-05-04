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

                if (ice::pod::array::empty(thread_tasks))
                {
                    SleepEx(1, 0);
                }
                else
                {
                    for (std::coroutine_handle<> coro_task : thread_tasks)
                    {
                        if (coro_task.resume(); coro_task.done())
                        {
                            coro_task.destroy();
                        }
                    }
                }
            }
        }

    private:
        std::thread _thread;
        bool _stop_requested = false;

        ice::TaskListContainer _task_lists;
    };

    auto create_task_thread(ice::Allocator& alloc) noexcept -> ice::UniquePtr<TaskThread>
    {
        return ice::make_unique<TaskThread, SimpleTaskThread>(alloc, alloc);
    }

    void detail::task_thread_routine(void* task_thread_object) noexcept
    {
        SimpleTaskThread* task_thread = reinterpret_cast<SimpleTaskThread*>(task_thread_object);

    }

} // namespace ice
