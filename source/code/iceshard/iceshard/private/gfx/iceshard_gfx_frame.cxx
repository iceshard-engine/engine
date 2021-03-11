#include "iceshard_gfx_frame.hxx"
#include "iceshard_gfx_pass.hxx"
#include <ice/render/render_swapchain.hxx>
#include <ice/assert.hxx>

namespace ice::gfx
{

    IceGfxFrame::IceGfxFrame(
        ice::render::RenderDevice* device,
        ice::render::RenderSwapchain* swapchain,
        ice::render::RenderPass renderpass,
        ice::render::Framebuffer framebuffer,
        ice::gfx::IceGfxPassGroup* pass_group
    ) noexcept
        : GfxFrame{ }
        , _render_device{ device }
        , _render_swapchain{ swapchain }
        , _pass_group{ pass_group }
    {
        ice::gfx::IceGfxPass* pass = _pass_group->get_pass(
            "default"_sid
        );

        pass->prepare();
        pass->alloc_command_buffers(
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
        ice::gfx::IceGfxPass* pass = _pass_group->get_pass(
            "default"_sid
        );
        ice::render::RenderQueue* queue = pass->render_queue();
        queue->submit(ice::Span<ice::render::CommandBuffer>(&_cmd_buffer, 1));

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
    ) noexcept -> GfxPass*
    {
        return _pass_group->get_pass(name);
    }

} // namespace ice::gfx
