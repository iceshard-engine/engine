#include "trait_render_finish.hxx"

#include <ice/engine_runner.hxx>
#include <ice/world/world_portal.hxx>

#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_resource_tracker.hxx>
#include <ice/gfx/gfx_frame.hxx>

#include <ice/render/render_command_buffer.hxx>
#include <ice/render/render_swapchain.hxx>

namespace ice
{

    IceWorldTrait_RenderFinish::IceWorldTrait_RenderFinish(ice::StringID_Arg stage_name) noexcept
        : _stage_name{ stage_name }
    {
    }

    void IceWorldTrait_RenderFinish::gfx_update(
        ice::EngineFrame const& engine_frame,
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept
    {
        gfx_frame.set_stage_slot(_stage_name, this);
    }

    void IceWorldTrait_RenderFinish::record_commands(
        ice::gfx::GfxContext const& context,
        ice::EngineFrame const& frame,
        ice::render::CommandBuffer cmds,
        ice::render::RenderCommands& api
    ) const noexcept
    {
        api.end_renderpass(cmds);
    }

} // namespace ice
