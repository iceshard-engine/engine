#pragma once
#include <ice/game_render_traits.hxx>
#include <ice/render/render_declarations.hxx>
#include <ice/gfx/gfx_device.hxx>

namespace ice
{

    class IceWorldTrait_RenderClear : public ice::gfx::GfxTrait, public ice::gfx::GfxStage
    {
    public:
        IceWorldTrait_RenderClear(ice::StringID_Arg stage_name) noexcept;

        void gfx_context_setup(
            ice::gfx::GfxDevice& device,
            ice::gfx::GfxContext& context
        ) noexcept override;

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
        ice::render::RenderSwapchain const* _default_swapchain;
        ice::render::Renderpass _default_renderpass;
        ice::render::Framebuffer _default_framebuffers[2];
    };

} // namespace ice
