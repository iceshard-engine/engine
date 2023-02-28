#include "task_thread_pool_impl.hxx"

namespace ice
{

    auto create_thread_pool(
        ice::Allocator& alloc,
        ice::TaskQueue_v3& queue,
        ice::TaskThreadPoolInfo_v3 const& threadpool_info
    ) noexcept -> ice::UniquePtr<ice::TaskThreadPool_v3>
    {
        return ice::make_unique<ice::TaskThreadPoolImplementation>(alloc, alloc, queue, threadpool_info);
    }

} // namespace ice
