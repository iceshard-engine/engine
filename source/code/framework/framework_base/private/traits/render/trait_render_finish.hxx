#pragma once
#include <ice/game_render_traits.hxx>
#include <ice/render/render_declarations.hxx>

namespace ice
{

    class IceWorldTrait_RenderFinish : public ice::GameWorldTrait_Render, public ice::gfx::GfxStage
    {
    public:
        auto gfx_stage_infos() const noexcept -> ice::Span<ice::gfx::GfxStageInfo const> override;
        auto gfx_stage_slots() const noexcept -> ice::Span<ice::gfx::GfxStageSlot const> override;

        void on_update(
            ice::EngineFrame& frame,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

        void record_commands(
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer cmds,
            ice::render::RenderCommands& api
        ) const noexcept override;
    };

} // namespace ice
