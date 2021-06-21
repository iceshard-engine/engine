#include "iceshard_gfx_runner.hxx"

namespace ice::gfx
{

    IceGfxRunner::IceGfxRunner(ice::Allocator& alloc) noexcept
        : _thread{ ice::make_unique_null<ice::TaskThread>() }
    {
        _thread = ice::create_task_thread(alloc);
    }

    IceGfxRunner::~IceGfxRunner() noexcept
    {
        _thread->stop();
        _thread->join();
    }

    void IceGfxRunner::push_task(ice::Task<void> task) noexcept
    {
        _thread->schedule(ice::move(task));
    }

} // namespace ice::gfx
