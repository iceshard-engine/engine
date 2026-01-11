/// Copyright 2023 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task_thread_pool.hxx>
#include <ice/task_flags.hxx>
#include <ice/array.hxx>
#include <ice/container/hashmap.hxx>
#include "task_native_thread.hxx"

namespace ice
{

    class TaskThreadPoolImplementation final : public ice::TaskThreadPool
    {
    public:
        TaskThreadPoolImplementation(
            ice::Allocator& alloc,
            ice::TaskQueue& queue,
            ice::TaskThreadPoolCreateInfo const& info
        ) noexcept;
        ~TaskThreadPoolImplementation() noexcept override;

        auto thread_count() const noexcept -> ice::u32 override;
        auto managed_thread_count() const noexcept -> ice::u32 override;
        auto estimated_task_count() const noexcept -> ice::u32 override;

        auto create_thread(ice::StringID name) noexcept -> ice::TaskThread& override;
        auto find_thread(ice::StringID name) noexcept -> ice::TaskThread* override;
        bool destroy_thread(ice::StringID name) noexcept override;

        auto attach_thread(
            ice::StringID name,
            //ice::TaskFlags accepting_flags,
            ice::UniquePtr<ice::TaskThread> thread
        ) noexcept -> ice::TaskThread& override;

        auto detach_thread(
            ice::StringID name
        ) noexcept -> ice::UniquePtr<ice::TaskThread> override;

    private:
        struct PoolThread
        {
            ice::TaskFlags accepting_flags{};
            ice::NativeTaskThread* native_thread = nullptr;
        };

        ice::Allocator& _allocator;
        ice::TaskQueue& _queue;
        ice::TaskThreadPoolCreateInfo const _info;

        ice::Array<PoolThread, ContainerLogic::Complex> _thread_pool;
        ice::Array<ice::UniquePtr<ice::NativeTaskThread>, ContainerLogic::Complex> _managed_threads;
        ice::HashMap<ice::UniquePtr<ice::NativeTaskThread>, ContainerLogic::Complex> _created_threads;
        ice::HashMap<ice::UniquePtr<ice::TaskThread>, ContainerLogic::Complex> _user_threads;
    };

} // namespace ice
