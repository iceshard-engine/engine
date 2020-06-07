#include "ice_rs_opaque.hxx"

#include <iceshard/renderer/render_api.hxx>
#include <iceshard/renderer/render_funcs.hxx>
#include <iceshard/renderer/render_commands.hxx>
#include <iceshard/render/render_pass.hxx>

namespace iceshard
{
    IceRS_Opaque::IceRS_Opaque(core::allocator& alloc) noexcept
        : IceRenderStage{ alloc }
    {
    }

    void IceRS_Opaque::on_prepare(
        iceshard::Engine& engine,
        iceshard::RenderPass& render_pass
    ) noexcept
    {
        using namespace iceshard::renderer;

        auto cmds_view = core::pod::array::create_view(&_command_buffer, 1);
        create_command_buffers(
            api::CommandBufferType::Secondary,
            cmds_view
        );
    }

    void IceRS_Opaque::on_cleanup(
        iceshard::Engine& engine,
        iceshard::RenderPass& render_pass
    ) noexcept
    {

    }

    void IceRS_Opaque::on_execute(
        iceshard::Frame& current,
        iceshard::RenderPass& render_pass
    ) noexcept
    {
        using namespace iceshard::renderer;
        namespace cmd = iceshard::renderer::commands;

        cmd::begin(_command_buffer, render_pass.handle(), 1);
        await_tasks(current, _command_buffer);
        cmd::end(_command_buffer);

        cmd::execute_commands(render_pass.command_buffer(), 1, (api::CommandBuffer*) &_command_buffer);
    }

} // namespace iceshard
