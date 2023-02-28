#pragma once
#include <ice/task_thread_pool_v3.hxx>
#include <ice/task_flags_v3.hxx>
#include <ice/container/array.hxx>
#include <ice/container/hashmap.hxx>
#include "task_native_thread.hxx"

namespace ice
{

    class TaskThreadPoolImplementation final : public ice::TaskThreadPool_v3
    {
    public:
        TaskThreadPoolImplementation(
            ice::Allocator& alloc,
            ice::TaskQueue_v3& queue,
            ice::TaskThreadPoolInfo_v3 const& info
        ) noexcept;
        ~TaskThreadPoolImplementation() noexcept override;

        auto thread_count() const noexcept -> ice::ucount override;
        auto managed_thread_count() const noexcept -> ice::ucount override;
        auto estimated_task_count() const noexcept -> ice::ucount override;

        auto create_thread(ice::StringID name) noexcept -> ice::TaskThread_v3& override;
        auto find_thread(ice::StringID name) noexcept -> ice::TaskThread_v3* override;
        bool destroy_thread(ice::StringID name) noexcept override;

        auto attach_thread(
            ice::StringID name,
            ice::TaskFlags accepting_flags,
            ice::UniquePtr<ice::TaskThread_v3> thread
        ) noexcept -> ice::TaskThread_v3& override;

        auto detach_thread(
            ice::StringID name
        ) noexcept -> ice::UniquePtr<ice::TaskThread_v3> override;

    private:
        struct PoolThread
        {
            ice::TaskFlags accepting_flags{};
            ice::NativeTaskThread* native_thread = nullptr;
        };

        ice::Allocator& _allocator;
        ice::TaskQueue_v3& _queue;
        ice::TaskThreadPoolInfo_v3 const _info;

        ice::Array<PoolThread, ContainerLogic::Complex> _thread_pool;
        ice::Array<ice::UniquePtr<ice::NativeTaskThread>, ContainerLogic::Complex> _managed_threads;
        ice::HashMap<ice::UniquePtr<ice::NativeTaskThread>, ContainerLogic::Complex> _created_threads;
        ice::HashMap<ice::UniquePtr<ice::TaskThread_v3>, ContainerLogic::Complex> _user_threads;
    };

} // namespace ice
