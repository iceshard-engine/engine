#pragma once
#include <ice/task.hxx>
#include <ice/collections.hxx>

namespace ice
{

    class IceshardTaskExecutor
    {
    public:
        IceshardTaskExecutor(
            ice::Allocator& alloc,
            ice::Vector<ice::Task<void>> tasks
        ) noexcept;
        ~IceshardTaskExecutor() noexcept = default;

        void wait_ready() noexcept;

    private:
        ice::Allocator& _allocator;
        ice::Vector<ice::Task<>> _tasks;
    };

}
