/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/gfx/gfx_stage.hxx>
#include <ice/mem_data.hxx>

#if 0
namespace ice
{

    class IceWorldTrait_RenderDebug : public ice::gfx::GfxTrait, public ice::gfx::GfxContextStage
    {
    public:
        void on_activate(
            ice::Engine& engine,
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

        void record_commands(
            ice::gfx::GfxContext const& context,
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer command_buffer,
            ice::render::RenderCommands& render_commands
        ) const noexcept override;

    private:
        ice::render::ResourceSetLayout _resource_layout;
        ice::render::ResourceSet _resource_set;
        ice::render::PipelineLayout _layout;
        ice::render::Pipeline _pipeline;

        ice::StringID _render_camera;
        ice::render::Buffer _render_camera_buffer;
        ice::render::Buffer _vertices;
        ice::render::Buffer _colors;

        ice::render::ShaderStageFlags _shader_stages[2];
        ice::render::Shader _shaders[2];
        ice::Data _shader_data[2];
    };


    class WorldTraitArchive;

    void register_trait_render_debug(
        ice::WorldTraitArchive& archive
    ) noexcept;

} // namespace ice
#endif
