#pragma once
#include <ice/game_render_traits.hxx>
#include <ice/gfx/gfx_device.hxx>
#include <ice/engine_runner.hxx>

namespace ice
{

    class AssetSystem;

    class IceWorldTrait_RenderPostProcess : public ice::GameWorldTrait_Render, public ice::gfx::GfxStage
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
        ) const noexcept override;

    protected:
        auto task_create_render_objects(
            ice::AssetSystem& asset_system,
            ice::EngineRunner& runner,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept -> ice::Task<>;

        auto task_update_resources(
            ice::gfx::GfxDevice& gfx_device
        ) noexcept -> ice::Task<>;

        auto task_destroy_render_objects(
            ice::gfx::GfxDevice& gfx_device
        ) noexcept -> ice::Task<>;

    private:
        ice::u32 _slot_count = 0;
        ice::render::ResourceSetLayout _resource_layout;
        ice::render::ResourceSet _resource_set;
        ice::render::PipelineLayout _layout;
        ice::render::Pipeline _pipeline;
        ice::render::Sampler _sampler;

        ice::render::Buffer _vertices;

        ice::render::ShaderStageFlags _shader_stages[2];
        ice::render::Shader _shaders[2];
    };

} // namespace ice
