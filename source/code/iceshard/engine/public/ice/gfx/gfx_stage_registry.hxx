/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/gfx/gfx_stage.hxx>

namespace ice::gfx
{

    struct GfxStageRegistry
    {
        virtual ~GfxStageRegistry() noexcept = default;

        virtual void add_stage(
            ice::StringID_Arg name,
            ice::gfx::GfxStage const* stage
        ) noexcept = 0;

        virtual void execute_stages(
            ice::EngineFrame const& frame,
            ice::StringID_Arg name,
            ice::render::CommandBuffer cmds,
            ice::render::RenderCommands& render_api
        ) const noexcept = 0;
    };

} // namespace ice::gfx
