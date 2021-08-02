#include "trait_render_clear.hxx"

#include <ice/engine_runner.hxx>
#include <ice/world/world_portal.hxx>

#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_resource_tracker.hxx>
#include <ice/gfx/gfx_frame.hxx>

#include <ice/render/render_command_buffer.hxx>
#include <ice/render/render_swapchain.hxx>

#include <ice/span_filter.hxx>
#include <ice/profiler.hxx>

namespace ice
{

    IceWorldTrait_RenderClear::IceWorldTrait_RenderClear(ice::StringID_Arg stage_name) noexcept
        : _stage_name{ stage_name }
    {
    }

    void IceWorldTrait_RenderClear::gfx_setup(
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept
    {
        IPT_ZONE_SCOPED_NAMED("[Trait] Clear :: Setup");

        using namespace ice::gfx;
        using namespace ice::render;

        GfxResourceTracker& gfx_restracker = gfx_device.resource_tracker();

        _default_swapchain = ice::addressof(gfx_device.swapchain());
        _default_renderpass = ice::gfx::find_resource<Renderpass>(gfx_restracker, "ice.gfx.renderpass.default"_sid);
        _default_framebuffers[0] = ice::gfx::find_resource<Framebuffer>(gfx_restracker, "ice.gfx.framebuffer.0"_sid);
        _default_framebuffers[1] = ice::gfx::find_resource<Framebuffer>(gfx_restracker, "ice.gfx.framebuffer.1"_sid);
    }

    void IceWorldTrait_RenderClear::gfx_update(
        ice::EngineFrame const& engine_frame,
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept
    {
        IPT_ZONE_SCOPED_NAMED("[Trait] Clear :: Update");

        for (ice::Shard const& shard : ice::filter_span(engine_frame.shards(), ice::any_of<ice::platform::Shard_WindowSizeChanged>))
        {
            gfx_setup(gfx_frame, gfx_device);
        }

        gfx_frame.set_stage_slot(_stage_name, this);
    }

    void IceWorldTrait_RenderClear::record_commands(
        ice::gfx::GfxContext const& context,
        ice::EngineFrame const& frame,
        ice::render::CommandBuffer cmds,
        ice::render::RenderCommands& api
    ) const noexcept
    {
        IPT_ZONE_SCOPED_NAMED("[Trait] Clear :: Graphics Commands");

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

        ice::vec4u scissor_and_viewport{
            0, 0,
            _default_swapchain->extent().x,
            _default_swapchain->extent().y
        };

        api.set_scissor(cmds, scissor_and_viewport);
        api.set_viewport(cmds, scissor_and_viewport);
        api.next_subpass(cmds, ice::render::SubPassContents::Inline);
    }

} // namespace ice
