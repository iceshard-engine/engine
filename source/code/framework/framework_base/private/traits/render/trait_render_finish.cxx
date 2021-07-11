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

    auto IceWorldTrait_RenderFinish::gfx_stage_infos() const noexcept -> ice::Span<ice::gfx::GfxStageInfo const>
    {
        static ice::StringID const dependencies[]{
            "frame.clear"_sid,
            "frame.render-sprites"_sid,
            "frame.render-postprocess"_sid,
        };
        static ice::gfx::GfxStageInfo const infos[]{
            ice::gfx::GfxStageInfo
            {
                .name = "frame.finish"_sid,
                .dependencies = dependencies,
                .type = ice::gfx::GfxStageType::DrawStage
            }
        };
        return infos;
    }

    auto IceWorldTrait_RenderFinish::gfx_stage_slots() const noexcept -> ice::Span<ice::gfx::GfxStageSlot const>
    {
        static ice::gfx::GfxStageSlot const slots[]{
            ice::gfx::GfxStageSlot
            {
                .name = "frame.finish"_sid,
                .stage = this
            }
        };
        return slots;
    }

    void IceWorldTrait_RenderFinish::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        ice::gfx::GfxFrame& gfx_frame = runner.graphics_frame();
        gfx_frame.set_stage_slots(gfx_stage_slots());
    }

    void IceWorldTrait_RenderFinish::record_commands(
        ice::EngineFrame const& frame,
        ice::render::CommandBuffer cmds,
        ice::render::RenderCommands& api
    ) const noexcept
    {
        api.end_renderpass(cmds);
    }

} // namespace ice
