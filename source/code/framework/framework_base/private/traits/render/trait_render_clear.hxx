#pragma once
#include <ice/game_render_traits.hxx>
#include <ice/render/render_declarations.hxx>
#include <ice/gfx/gfx_device.hxx>

namespace ice
{

    class IceWorldTrait_RenderClear : public ice::GameWorldTrait_Render, public ice::gfx::GfxStage
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

        void record_commands(
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer cmds,
            ice::render::RenderCommands& api
        ) noexcept override;

    protected:
        auto task_activate_graphics(
            ice::EngineRunner& runner,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept -> ice::Task<>;

    private:
        ice::u32 _stage_slot_count = 0;
        ice::gfx::GfxStageSlot _stage_slots[1];

        ice::render::RenderSwapchain const* _default_swapchain;
        ice::render::Renderpass _default_renderpass;
        ice::render::Framebuffer _default_framebuffers[2];
    };

} // namespace ice
