#include "iceshard_gfx_frame.hxx"
#include "iceshard_gfx_queue.hxx"
#include <ice/pod/hash.hxx>
#include <ice/render/render_swapchain.hxx>
#include <ice/task.hxx>
#include <ice/assert.hxx>

#include "../iceshard_task_executor.hxx"

namespace ice::gfx
{

    IceGfxFrame::IceGfxFrame(
        ice::Allocator& alloc
    ) noexcept
        : _allocator{ alloc, "gfx-frame" }
        , _enqueued_passes{ _allocator }
        , _frame_tasks{ _allocator }
    {
        _frame_tasks.reserve(1024);
    }

    IceGfxFrame::~IceGfxFrame() noexcept
    {
    }

    void IceGfxFrame::execute_task(ice::Task<void> task) noexcept
    {
        _frame_tasks.push_back(ice::move(task));
    }

    void IceGfxFrame::wait_ready() noexcept
    {
        IceshardTaskExecutor{ _allocator, ice::move(_frame_tasks) }.wait_ready();
    }

    void IceGfxFrame::enqueue_pass(
        ice::StringID_Arg queue_name,
        ice::gfx::GfxPass* pass
    ) noexcept
    {
        ice::pod::multi_hash::insert(
            _enqueued_passes,
            ice::hash(queue_name),
            pass
        );
    }

    void IceGfxFrame::execute_passes(
        ice::EngineFrame const& frame,
        ice::gfx::IceGfxQueueGroup& queue_group
    ) noexcept
    {
        for (auto const& entry : _enqueued_passes)
        {
            ice::gfx::IceGfxQueue* const queue = queue_group.get_queue(ice::StringID{ StringID_Hash{ entry.key } });
            if (queue != nullptr)
            {
                queue->execute_pass(frame, entry.value);
            }
        }

        ice::pod::hash::clear(_enqueued_passes);
    }

} // namespace ice::gfx
