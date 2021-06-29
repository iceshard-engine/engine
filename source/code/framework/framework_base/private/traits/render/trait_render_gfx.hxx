#pragma once
#include <ice/game_render_traits.hxx>
#include <ice/render/render_declarations.hxx>

namespace ice
{

    class IceWorldTrait_RenderGfx : public ice::GameWorldTrait_Render
    {
    public:
        auto gfx_stage_infos() const noexcept -> ice::Span<ice::gfx::GfxStageInfo const> override;
        auto gfx_stage_slots() const noexcept -> ice::Span<ice::gfx::GfxStageSlot const> override;

        void on_activate(
            ice::Engine& engine,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

        void on_deactivate(
            ice::Engine& engine,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

        void on_update(
            ice::EngineFrame& frame,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

    private:
        ice::render::Renderpass _default_renderpass;
        ice::render::Framebuffer _default_framebuffers[2];
        ice::render::Image _default_attachment_depth_stencil;
        ice::render::Image _default_attachment_color;
    };

} // namespace ice
