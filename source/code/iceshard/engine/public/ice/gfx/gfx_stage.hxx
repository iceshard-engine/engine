/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/gfx/gfx_types.hxx>
#include <ice/asset_storage.hxx>
#include <ice/task.hxx>

namespace ice::gfx
{

    struct GfxStage
    {
        virtual ~GfxStage() noexcept = default;

        virtual auto initialize(
            ice::gfx::GfxDevice& gfx,
            ice::gfx::GfxStages& stages,
            ice::render::Renderpass renderpass
        ) noexcept -> ice::Task<> { co_return; }

        virtual auto cleanup(
            ice::gfx::GfxDevice& gfx
        ) noexcept -> ice::Task<> { co_return; }

        virtual void update(
            ice::gfx::GfxDevice& gfx
        ) noexcept { }

        virtual void draw(
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer cmds,
            ice::render::RenderCommands& render_api
        ) const noexcept = 0;
    };

} // namespace ice::gfx
