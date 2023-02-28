#pragma once
#include <ice/task_thread_v3.hxx>
#include <ice/task_awaitable_v3.hxx>
#include <ice/container/linked_queue.hxx>

namespace ice
{

    class NativeTaskThread;

    namespace thread_native
    {

        static constexpr ice::u32 Constant_BusyLoopCount = 200;

        using Handle = struct { void* native; };

        auto create_thread(ice::NativeTaskThread& thread) noexcept -> Handle;
        void destroy_thread(Handle native_handle) noexcept;

        void sleep(ice::u32 ms) noexcept;
        void yield() noexcept;

    } // namespace native

    enum class ThreadState : uint8_t
    {
        Invalid,
        Active,
        Destroyed,
    };

    enum class ThreadRequest : uint8_t
    {
        None,
        Create,
        Destroy,
    };

    struct ThreadRuntime
    {
        ice::TaskThreadInfo const& _info;
        ice::TaskQueue_v3& _queue;
        ice::ThreadState _state = ThreadState::Invalid;
        ice::ThreadRequest _request = ThreadRequest::Destroy;

        using RoutineFn = auto (ThreadRuntime::*)() noexcept -> ice::u32;

        auto thread_procedure(RoutineFn routine) noexcept -> ice::u32;

        auto shared_routine() noexcept -> ice::u32;
        auto exclusive_fifo_routine() noexcept -> ice::u32;
        auto exclusive_sorted_routine() noexcept -> ice::u32;
    };

    class NativeTaskThread final : public ice::TaskThread_v3
    {
    public:
        NativeTaskThread(
            ice::TaskQueue_v3& queue,
            ice::TaskThreadInfo const& info
        ) noexcept;

        ~NativeTaskThread() noexcept override;

        bool valid() noexcept;
        auto runtime() noexcept -> ice::ThreadRuntime&;
        auto info() const noexcept -> ice::TaskThreadInfo const& override;

        bool is_busy() const noexcept override;
        bool is_running() const noexcept override;

        auto estimated_task_count() const noexcept -> ice::ucount override;
        auto queue() noexcept -> ice::TaskQueue_v3 & override;

    private:
        ice::TaskThreadInfo const _info;
        ice::ThreadRuntime _runtime;

        ice::thread_native::Handle _native;
    };

} // namespace ice
