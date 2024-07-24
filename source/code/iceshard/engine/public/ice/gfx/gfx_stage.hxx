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
            ice::gfx::GfxContext& gfx,
            ice::gfx::GfxFrameStages& stages,
            ice::render::Renderpass renderpass,
            ice::u32 subpass
        ) noexcept -> ice::Task<> { co_return; }

        virtual auto cleanup(
            ice::gfx::GfxContext& gfx
        ) noexcept -> ice::Task<> { co_return; }

        virtual void update(
            ice::EngineFrame const& frame,
            ice::gfx::GfxContext& gfx
        ) noexcept { }

        virtual void draw(
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer cmds,
            ice::render::RenderCommands& render_api
        ) const noexcept = 0;
    };

} // namespace ice::gfx
