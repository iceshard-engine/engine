#include "trait_render_finish.hxx"

#include <ice/engine_runner.hxx>
#include <ice/world/world_portal.hxx>
#include <ice/world/world_trait_archive.hxx>

#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_resource_tracker.hxx>
#include <ice/gfx/gfx_frame.hxx>

#include <ice/render/render_command_buffer.hxx>
#include <ice/render/render_swapchain.hxx>

namespace ice
{

    void IceWorldTrait_RenderFinish::gfx_update(
        ice::EngineFrame const& engine_frame,
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept
    {
        gfx_frame.set_stage_slot(ice::Constant_GfxStage_Finish, this);
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

    void register_trait_render_finish(
        ice::WorldTraitArchive& archive
    ) noexcept
    {
        static constexpr ice::StringID trait_dependencies[]{
            Constant_TraitName_RenderPostprocess,
        };

        archive.register_trait(
            ice::Constant_TraitName_RenderFinish,
            ice::WorldTraitDescription
            {
                .factory = ice::detail::generic_trait_factory<IceWorldTrait_RenderFinish>,
                .required_dependencies = trait_dependencies
            }
        );
    }

} // namespace ice
