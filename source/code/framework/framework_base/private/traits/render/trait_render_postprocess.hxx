#pragma once
#include <ice/game_render_traits.hxx>
#include <ice/gfx/gfx_device.hxx>
#include <ice/engine_runner.hxx>

namespace ice
{

    class AssetSystem;

    class IceWorldTrait_RenderPostProcess : public ice::gfx::GfxTrait, public ice::gfx::GfxStage
    {
    public:
        IceWorldTrait_RenderPostProcess(ice::StringID_Arg stage_name) noexcept;

        void gfx_context_setup(
            ice::gfx::GfxDevice& device,
            ice::gfx::GfxContext& context
        ) noexcept override;

        void gfx_context_cleanup(
            ice::gfx::GfxDevice& device,
            ice::gfx::GfxContext& context
        ) noexcept override;

        void gfx_update(
            ice::EngineFrame const& engine_frame,
            ice::gfx::GfxDevice& device,
            ice::gfx::GfxContext& context,
            ice::gfx::GfxFrame& frame
        ) noexcept override;

        void on_activate(
            ice::Engine& engine,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

        void record_commands(
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer cmds,
            ice::render::RenderCommands& api
        ) const noexcept override;

    protected:
        void update_resources(
            ice::gfx::GfxDevice& gfx_device
        ) noexcept;

    private:
        ice::StringID const _stage_name;
        ice::render::ResourceSetLayout _resource_layout;
        ice::render::ResourceSet _resource_set;
        ice::render::PipelineLayout _layout;
        ice::render::Pipeline _pipeline;
        ice::render::Sampler _sampler;

        ice::render::Buffer _vertices;

        ice::render::ShaderStageFlags _shader_stages[2];
        ice::render::Shader _shaders[2];
        ice::Data _shader_data[2];
    };

} // namespace ice
