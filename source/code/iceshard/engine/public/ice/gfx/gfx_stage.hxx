/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/engine_frame.hxx>
#include <ice/render/render_declarations.hxx>

namespace ice::gfx
{

    class GfxDevice;
    class GfxContext;

    class GfxContextStage
    {
    public:
        virtual ~GfxContextStage() noexcept = default;

        virtual void prepare_context(
            ice::gfx::GfxContext& context,
            ice::gfx::GfxDevice& device
        ) const noexcept { }

        virtual void clear_context(
            ice::gfx::GfxContext& context,
            ice::gfx::GfxDevice& device
        ) const noexcept { }

        virtual void record_commands(
            ice::gfx::GfxContext const& context,
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer command_buffer,
            ice::render::RenderCommands& render_commands
        ) const noexcept = 0;
    };

    class GfxFrameStage
    {
    public:
        virtual ~GfxFrameStage() noexcept = default;

        virtual void record_commands(
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer command_buffer,
            ice::render::RenderCommands& render_commands
        ) const noexcept = 0;
    };

    namespace v2
    {

        struct GfxObjectStorage;
        struct GfxRenderGraphRuntime;

        struct GfxStageState
        {
            ice::render::Renderpass renderpass;
            ice::render::Framebuffer framebuffer;
            ice::math::vec2u framebuffer_size;
        };

        struct GfxStageParams
        {
            ice::EngineFrame const& frame;
            ice::gfx::GfxDevice& device;
            ice::gfx::v2::GfxStageState const& state;
            ice::render::CommandBuffer command_buffer;
            ice::render::RenderCommands& render_commands;
        };

        //struct GfxResources
        //{
        //    virtual ~GfxResources() noexcept = default;

        //    virtual bool find(ice::StringID_Arg name, ice::render::Renderpass& out_renderpass) const noexcept = 0;
        //    virtual bool find(ice::StringID_Arg name, ice::render::Framebuffer& out_framebuffer) const noexcept = 0;
        //    virtual bool find(ice::StringID_Arg name, ice::render::Image& out_image) const noexcept = 0;

        //    virtual void track(ice::StringID_Arg name, ice::render::Renderpass renderpass) noexcept = 0;
        //    virtual void track(ice::StringID_Arg name, ice::render::Framebuffer framebuffer) noexcept = 0;
        //    virtual void track(ice::StringID_Arg name, ice::render::Image framebuffer) noexcept = 0;
        //};

        struct GfxStage
        {
            virtual ~GfxStage() noexcept = default;

            virtual void update(
                ice::gfx::v2::GfxStageParams const& params,
                ice::gfx::v2::GfxStageDefinition const& def,
                ice::gfx::v2::GfxObjectStorage& resources
            ) noexcept;

            virtual void execute(
                ice::gfx::v2::GfxStageParams const& params,
                ice::gfx::v2::GfxObjectStorage& resources
            ) noexcept = 0;
        };

    } // namespace v2

} // namespace ice::gfx
