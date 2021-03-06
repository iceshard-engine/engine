#include "iceshard_gfx_frame.hxx"
#include <ice/render/render_swapchain.hxx>

namespace ice::gfx
{

    IceGfxFrame::IceGfxFrame(
        ice::render::RenderDevice* device,
        ice::render::RenderSwapchain* swapchain,
        ice::render::RenderPass renderpass,
        ice::render::Framebuffer framebuffer,
        ice::render::RenderQueue* queue,
        ice::u32 queue_pool_idx
    ) noexcept
        : GfxFrame{ }
        , _render_device{ device }
        , _render_swapchain{ swapchain }
        , _render_queue{ queue }
    {
        _render_queue->reset_pool(queue_pool_idx);
        _render_queue->allocate_buffers(
            queue_pool_idx,
            ice::render::CommandBufferType::Primary,
            ice::Span<ice::render::CommandBuffer>(&_cmd_buffer, 1)
        );

        ice::vec4f const clear_color{ 0.2f };

        ice::render::RenderCommands& cmds = _render_device->get_commands();
        cmds.begin(_cmd_buffer);
        cmds.begin_renderpass(_cmd_buffer, renderpass, framebuffer, swapchain->extent(), clear_color);
        cmds.next_subpass(_cmd_buffer, ice::render::SubPassContents::Inline);
        cmds.end_renderpass(_cmd_buffer);
        cmds.end(_cmd_buffer);
    }

    IceGfxFrame::~IceGfxFrame() noexcept
    {
    }

    void IceGfxFrame::present() noexcept
    {
        _render_queue->submit(ice::Span<ice::render::CommandBuffer>(&_cmd_buffer, 1));
        _render_queue->present(_render_swapchain);
    }

} // namespace ice::gfx
