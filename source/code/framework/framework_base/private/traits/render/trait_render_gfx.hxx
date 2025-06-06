/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/game_render_traits.hxx>
#include <ice/render/render_declarations.hxx>

#if 0
namespace ice
{

    class IceWorldTrait_RenderGfx : public ice::gfx::GfxTrait
    {
    public:
        void on_update(
            ice::EngineFrame& frame,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

        void gfx_setup(
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxContext& gfx_ctx
        ) noexcept override;

        void gfx_cleanup(
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxContext& gfx_ctx
        ) noexcept override;

        void gfx_update(
            ice::EngineFrame const& engine_frame,
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxContext& gfx_ctx
        ) noexcept override;

    private:
        ice::render::Renderpass _default_renderpass;
        ice::render::Framebuffer _default_framebuffers[2];
        ice::render::Image _default_attachment_depth_stencil;
        ice::render::Image _default_attachment_color;
    };


    class WorldTraitArchive;

    void register_trait_render_gfx(
        ice::WorldTraitArchive& archive
    ) noexcept;

} // namespace ice
#endif
