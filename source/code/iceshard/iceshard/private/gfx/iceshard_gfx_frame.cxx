#include "iceshard_gfx_frame.hxx"
#include "iceshard_gfx_queue.hxx"
#include <ice/render/render_swapchain.hxx>
#include <ice/assert.hxx>

namespace ice::gfx
{

    IceGfxFrame::IceGfxFrame(
        ice::render::RenderDevice* device,
        ice::render::RenderSwapchain* swapchain,
        ice::gfx::IceGfxQueueGroup* pass_group
    ) noexcept
        : GfxFrame{ }
        , _render_device{ device }
        , _render_swapchain{ swapchain }
        , _pass_group{ pass_group }
    {
        _pass_group->prepare_all();
    }

    IceGfxFrame::~IceGfxFrame() noexcept
    {
    }

    void IceGfxFrame::present() noexcept
    {
        _pass_group->execute_all();

        ice::render::RenderQueue* presenting_queue;
        if (_pass_group->get_presenting_queue(presenting_queue))
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

    auto IceGfxFrame::get_pass(
        ice::StringID_Arg name
    ) noexcept -> GfxQueue*
    {
        return _pass_group->get_queue(name);
    }

} // namespace ice::gfx
