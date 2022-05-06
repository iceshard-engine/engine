#pragma once
#include <ice/game_render_traits.hxx>
#include <ice/render/render_declarations.hxx>

namespace ice
{

    class IceWorldTrait_RenderFinish : public ice::gfx::GfxTrait, public ice::gfx::GfxContextStage
    {
    public:
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
    };


    class WorldTraitArchive;

    void register_trait_render_finish(
        ice::WorldTraitArchive& archive
    ) noexcept;

} // namespace ice
