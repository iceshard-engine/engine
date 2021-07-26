#pragma once
#include <ice/game_render_traits.hxx>
#include <ice/render/render_declarations.hxx>

namespace ice
{

    class IceWorldTrait_RenderFinish : public ice::gfx::GfxTrait, public ice::gfx::GfxStage
    {
    public:
        IceWorldTrait_RenderFinish(ice::StringID_Arg stage_name) noexcept;

        void gfx_update(
            ice::EngineFrame const& engine_frame,
            ice::gfx::GfxDevice& device,
            ice::gfx::GfxContext& context,
            ice::gfx::GfxFrame& frame
        ) noexcept override;

        void record_commands(
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer cmds,
            ice::render::RenderCommands& api
        ) const noexcept override;

    private:
        ice::StringID const _stage_name;
    };

} // namespace ice
