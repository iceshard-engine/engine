#pragma once
#include <ice/game_render_traits.hxx>
#include <ice/render/render_declarations.hxx>

namespace ice
{

    class IceWorldTrait_RenderFinish : public ice::gfx::GfxTrait, public ice::gfx::GfxContextStage
    {
    public:
        IceWorldTrait_RenderFinish(ice::StringID_Arg stage_name) noexcept;

        void gfx_update(
            ice::EngineFrame const& engine_frame,
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept override;

        void record_commands(
            ice::gfx::GfxContext const& context,
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer command_buffer,
            ice::render::RenderCommands& render_commands
        ) const noexcept override;

    private:
        ice::StringID const _stage_name;
    };

} // namespace ice
