#include "iceshard_gfx_frame.hxx"
#include "iceshard_gfx_queue.hxx"
#include <ice/pod/hash.hxx>
#include <ice/render/render_swapchain.hxx>
#include <ice/task.hxx>
#include <ice/assert.hxx>

#include "../iceshard_task_executor.hxx"

namespace ice::gfx
{

    IceGfxBaseFrame::IceGfxBaseFrame(
        ice::gfx::IceGfxQueueGroup* queue_group
    ) noexcept
        : _queue_group{ queue_group }
    {
    }

    auto IceGfxBaseFrame::get_queue(
        ice::StringID_Arg name
    ) noexcept -> GfxQueue*
    {
        return _queue_group->get_queue(name);
    }

    IceGfxFrame::IceGfxFrame(
        ice::Allocator& alloc,
        ice::render::RenderDevice* device,
        ice::render::RenderSwapchain* swapchain,
        ice::gfx::IceGfxQueueGroup* queue_group
    ) noexcept
        : IceGfxBaseFrame{ queue_group }
        , _allocator{ alloc, "gfx-frame" }
        , _render_device{ device }
        , _render_swapchain{ swapchain }
        , _enqueued_passes{ _allocator }
        , _frame_tasks{ _allocator }
    {
        _queue_group->prepare_all();
        _frame_tasks.reserve(1024);
    }

    IceGfxFrame::~IceGfxFrame() noexcept
    {
    }

    void IceGfxFrame::present() noexcept
    {
        //_pass_group->execute_all();

        ice::render::RenderQueue* presenting_queue;
        if (_queue_group->get_presenting_queue(presenting_queue))
        {
            presenting_queue->present(_render_swapchain);
        }
        else
        {
            ICE_ASSERT(
                false, "No graphics pass set as presenting!"
            );
        }
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
        ice::EngineFrame const& frame
    ) noexcept
    {
        for (auto const& entry : _enqueued_passes)
        {
            ice::gfx::IceGfxQueue* const queue = _queue_group->get_queue(ice::StringID{ StringID_Hash{ entry.key } });
            if (queue != nullptr)
            {
                queue->execute_pass(frame, entry.value);
            }
        }

        ice::pod::hash::clear(_enqueued_passes);
    }

} // namespace ice::gfx
