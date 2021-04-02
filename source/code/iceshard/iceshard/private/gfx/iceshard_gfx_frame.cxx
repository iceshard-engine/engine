#include "iceshard_gfx_frame.hxx"
#include "iceshard_gfx_queue.hxx"
#include <ice/render/render_swapchain.hxx>
#include <ice/assert.hxx>

namespace ice::gfx
{

    IceGfxFrame::IceGfxFrame(
        ice::render::RenderDevice* device,
        ice::render::RenderSwapchain* swapchain,
        ice::gfx::IceGfxQueueGroup* queue_group
    ) noexcept
        : IceGfxBaseFrame{ }
        , _render_device{ device }
        , _render_swapchain{ swapchain }
        , _queue_group{ queue_group }
    {
        _queue_group->prepare_all();
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

    auto IceGfxFrame::get_queue(
        ice::StringID_Arg name
    ) noexcept -> GfxQueue*
    {
        return _queue_group->get_queue(name);
    }

} // namespace ice::gfx
