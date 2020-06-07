#include "ice_rs_debugui.hxx"
#include <core/pod/array.hxx>
#include <iceshard/renderer/render_api.hxx>
#include <iceshard/renderer/render_funcs.hxx>
#include <iceshard/renderer/render_commands.hxx>
#include <iceshard/render/render_pass.hxx>

namespace iceshard
{

    IceRS_DebugUI::IceRS_DebugUI(core::allocator& alloc) noexcept
        : IceRenderStage{ alloc }
    {
    }

    void IceRS_DebugUI::on_prepare(
        iceshard::Engine& engine,
        iceshard::RenderPass& render_pass) noexcept
    {
        using namespace iceshard::renderer;

        auto cmds_view = core::pod::array::create_view(&_command_buffer, 1);
        create_command_buffers(
            api::CommandBufferType::Secondary,
            cmds_view
        );
    }

    void IceRS_DebugUI::on_cleanup(
        iceshard::Engine& engine,
        iceshard::RenderPass& render_pass) noexcept
    {
    }

    void IceRS_DebugUI::on_execute(
        iceshard::Frame& current,
        iceshard::RenderPass& render_pass
    ) noexcept
    {
        namespace cmd = iceshard::renderer::commands;
        using namespace iceshard::renderer;

        cmd::begin(_command_buffer, render_pass.handle(), 2);
        await_tasks(current, _command_buffer);
        cmd::end(_command_buffer);
        auto cb = render_pass.command_buffer();
        cmd::execute_commands(cb, 1, (api::CommandBuffer*) & _command_buffer);
    }

} // namespace iceshard
