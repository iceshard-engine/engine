#include "task_native_thread.hxx"

namespace ice
{

    auto create_thread(
        ice::Allocator& alloc,
        ice::TaskQueue_v3& queue,
        ice::TaskThreadInfo const& thread_info
    ) noexcept -> ice::UniquePtr<ice::TaskThread_v3>
    {
        ice::UniquePtr<ice::NativeTaskThread> thread = ice::make_unique<ice::NativeTaskThread>(
            alloc,
            queue,
            thread_info
        );

        if (thread->valid())
        {
            return thread;
        }
        return { };
    }

} // namespace ice
