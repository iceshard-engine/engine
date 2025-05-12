/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/game_render_traits.hxx>
#include <ice/gfx/gfx_context.hxx>
#include <ice/engine_runner.hxx>

namespace ice
{

#if 0
    class IceWorldTrait_RenderPostProcess : public ice::gfx::GfxTrait, public ice::gfx::GfxContextStage
    {
    public:
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

        void on_activate(
            ice::Engine& engine,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

        void record_commands(
            ice::gfx::GfxContext const& context,
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer command_buffer,
            ice::render::RenderCommands& render_commands
        ) const noexcept override;

    protected:
        void update_resources(
            ice::gfx::GfxContext& gfx_ctx
        ) noexcept;

    private:
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


    class WorldTraitArchive;

    void register_trait_render_postprocess(
        ice::WorldTraitArchive& archive
    ) noexcept;
#endif

} // namespace ice
