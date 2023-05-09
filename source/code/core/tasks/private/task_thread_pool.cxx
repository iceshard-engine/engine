#include "task_thread_pool_impl.hxx"

namespace ice
{

    auto create_thread_pool(
        ice::Allocator& alloc,
        ice::TaskQueue& queue,
        ice::TaskThreadPoolCreateInfo const& threadpool_info
    ) noexcept -> ice::UniquePtr<ice::TaskThreadPool>
    {
        return ice::make_unique<ice::TaskThreadPoolImplementation>(alloc, alloc, queue, threadpool_info);
    }

} // namespace ice
