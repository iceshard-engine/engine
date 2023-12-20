/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/gfx/gfx_types.hxx>

namespace ice::gfx
{

    struct GfxStage
    {
        virtual ~GfxStage() noexcept = default;

        virtual void draw(
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer cmds,
            ice::render::RenderCommands& render_api
        ) const noexcept = 0;
    };

} // namespace ice::gfx
