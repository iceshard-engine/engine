#include <ice/task_thread.hxx>
#include <ice/task_list.hxx>
#include <ice/task.hxx>
#include <ice/os/windows.hxx>
#include <ice/memory/stack_allocator.hxx>
#include <thread>

#include "internal_task.hxx"

namespace ice
{

    class SimpleTaskThread_v2 : public ice::TaskThread_v2
    {
    public:
        using ScheduleOperation = ice::ScheduleOperation<ice::TaskThread>;

        SimpleTaskThread_v2(ice::Allocator& alloc) noexcept
            : _thread{ }
        {
            _thread = std::thread{ std::bind(&SimpleTaskThread_v2::thread_routine, this) };
        }

        ~SimpleTaskThread_v2() noexcept override
        {
            join();

            // Destroy any tasks that are still waiting for processing
            ice::TaskOperation_v2* expected_initial_task = _head.load(std::memory_order_acquire);
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
                    ice::TaskOperation_v2* next_op = expected_initial_task->_next;
                    expected_initial_task->_coro.destroy();
                    expected_initial_task = next_op;
                }
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

        bool schedule(ice::TaskOperation_v2& operation) noexcept
        {
            ice::TaskOperation_v2* expected_head = _head.load(std::memory_order_acquire);

            do
            {
                operation._next = expected_head;
            }
            while (
                _head.compare_exchange_weak(
                    expected_head,
                    &operation,
                    std::memory_order_release,
                    std::memory_order_acquire
                ) == false
            );

            return true;
        }

    protected:
        void thread_routine() noexcept
        {
            while (_stop_requested == false)
            {
                ice::TaskOperation_v2* expected_initial_task = _head.load(std::memory_order_acquire);
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

                if (expected_initial_task != nullptr)
                {
                    while (expected_initial_task != nullptr)
                    {
                        ice::TaskOperation_v2* next_op = expected_initial_task->_next;

                        std::coroutine_handle<> coro_task = expected_initial_task->_coro;
                        coro_task.resume();
                        expected_initial_task = next_op;
                    }
                }
            }
        }

    private:
        std::thread _thread;
        bool _stop_requested = false;

        std::atomic<ice::TaskOperation_v2*> _head = nullptr;
    };

    auto create_task_thread_v2(ice::Allocator& alloc) noexcept -> ice::UniquePtr<TaskThread_v2>
    {
        return ice::make_unique<TaskThread_v2, SimpleTaskThread_v2>(alloc, alloc);
    }

} // namespace ice
