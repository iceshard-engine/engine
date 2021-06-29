#include "trait_render_clear.hxx"

#include <ice/engine_runner.hxx>
#include <ice/world/world_portal.hxx>

#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_resource_tracker.hxx>
#include <ice/gfx/gfx_frame.hxx>

#include <ice/render/render_command_buffer.hxx>
#include <ice/render/render_swapchain.hxx>

namespace ice
{

    auto IceWorldTrait_RenderClear::gfx_stage_infos() const noexcept -> ice::Span<ice::gfx::GfxStageInfo const>
    {
        static ice::gfx::GfxStageInfo const infos[]{
            ice::gfx::GfxStageInfo
            {
                .name = "frame.clear"_sid,
                .dependencies = {},
                .type = ice::gfx::GfxStageType::DrawStage
        }
        };
        return infos;
    }

    auto IceWorldTrait_RenderClear::gfx_stage_slots() const noexcept -> ice::Span<ice::gfx::GfxStageSlot const>
    {
        return { _stage_slots, _stage_slot_count };
    }

    void IceWorldTrait_RenderClear::on_activate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        ice::gfx::GfxDevice& gfx_device = runner.graphics_device();

        _default_swapchain = &gfx_device.swapchain();

        portal.execute(task_activate_graphics(runner, gfx_device));
    }

    void IceWorldTrait_RenderClear::on_deactivate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        _stage_slot_count = 0;
    }

    void IceWorldTrait_RenderClear::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        ice::gfx::GfxFrame& gfx_frame = runner.graphics_frame();
        gfx_frame.set_stage_slots(gfx_stage_slots());
    }

    void IceWorldTrait_RenderClear::record_commands(
        ice::EngineFrame const& frame,
        ice::render::CommandBuffer cmds,
        ice::render::RenderCommands& api
    ) noexcept
    {
        ice::vec4f clear_values[4]
        {
            ice::vec4f{ 0.3f },
            ice::vec4f{ 0.3f },
            ice::vec4f{ 0.3f },
            ice::vec4f{ 0.3f },
        };

        api.begin_renderpass(
            cmds,
            _default_renderpass,
            _default_framebuffers[_default_swapchain->current_image_index()],
            clear_values,
            _default_swapchain->extent()
        );
        api.next_subpass(cmds, ice::render::SubPassContents::Inline);
        api.next_subpass(cmds, ice::render::SubPassContents::Inline);
        api.end_renderpass(cmds);
    }

    auto IceWorldTrait_RenderClear::task_activate_graphics(
        ice::EngineRunner& runner,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept -> ice::Task<>
    {
        co_await runner.graphics_frame().frame_start();

        using namespace ice::gfx;
        using namespace ice::render;

        GfxResourceTracker& gfx_restracker = gfx_device.resource_tracker();
        _default_renderpass = ice::gfx::find_resource<Renderpass>(gfx_restracker, "ice.gfx.renderpass.default"_sid);
        _default_framebuffers[0] = ice::gfx::find_resource<Framebuffer>(gfx_restracker, "ice.gfx.framebuffer.0"_sid);
        _default_framebuffers[1] = ice::gfx::find_resource<Framebuffer>(gfx_restracker, "ice.gfx.framebuffer.1"_sid);

        co_await runner.schedule_next_frame();

        _stage_slot_count = 1;
        _stage_slots[0] = ice::gfx::GfxStageSlot
        {
            .name = "frame.clear"_sid,
            .stage = this
        };
    }

} // namespace ice
