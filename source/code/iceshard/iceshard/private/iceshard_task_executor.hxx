/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task.hxx>
#include <ice/sync_manual_events.hxx>
#include <ice/container_types.hxx>

namespace ice
{

    class ManualResetEvent;

    class IceshardTaskExecutor
    {
    public:
        using TaskList = ice::Array<ice::Task<void>, ice::ContainerLogic::Complex>;

        IceshardTaskExecutor(
            ice::Allocator& alloc,
            TaskList tasks
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
        ice::ucount _task_count;
        ice::Array<ice::Task<>, ice::ContainerLogic::Complex> _tasks;
        ice::ManualResetBarrier _sync_sem;
    };

}
