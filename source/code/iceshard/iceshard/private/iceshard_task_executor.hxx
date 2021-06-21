#pragma once
#include <ice/task.hxx>
#include <ice/collections.hxx>

namespace ice
{

    class ManualResetEvent;

    class IceshardTaskExecutor
    {
    public:
        IceshardTaskExecutor(
            ice::Allocator& alloc,
            ice::Vector<ice::Task<void>> tasks
        ) noexcept;

        IceshardTaskExecutor(IceshardTaskExecutor&& other) noexcept;
        auto operator=(IceshardTaskExecutor&& other) noexcept -> IceshardTaskExecutor&;

        IceshardTaskExecutor(IceshardTaskExecutor const& other) noexcept = delete;
        auto operator=(IceshardTaskExecutor const& other) noexcept -> IceshardTaskExecutor & = delete;

        ~IceshardTaskExecutor() noexcept;

        void start_all() noexcept;
        void wait_ready() noexcept;

    private:
        ice::Allocator& _allocator;
        ice::u32 _task_count;
        ice::Vector<ice::Task<>> _tasks;

        void* _reset_events_memory;
        ice::ManualResetEvent* _reset_events;
    };

}
