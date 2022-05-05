#pragma once
#include <ice/game_render_traits.hxx>
#include <ice/render/render_declarations.hxx>
#include <ice/gfx/gfx_device.hxx>

namespace ice
{

    class IceWorldTrait_RenderClear : public ice::gfx::GfxTrait, public ice::gfx::GfxContextStage
    {
    public:
        void gfx_setup(
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept override;

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
        ice::render::RenderSwapchain const* _default_swapchain;
        ice::render::Renderpass _default_renderpass;
        ice::render::Framebuffer _default_framebuffers[2];
    };


    class WorldTraitArchive;

    void register_trait_render_clear(
        ice::WorldTraitArchive& archive
    ) noexcept;

} // namespace ice
