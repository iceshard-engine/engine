/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/gfx/gfx_shards.hxx>

namespace ice::gfx
{

    struct QueueDefinition
    {
        ice::StringID name;
        ice::render::QueueFlags flags;
    };

    struct GfxRunnerCreateInfo
    {
        ice::Engine& engine;
        ice::render::RenderDriver& driver;
        ice::render::RenderSurface& surface;
        ice::Span<ice::gfx::QueueDefinition const> render_queues;
    };

    struct GfxStages
    {
        ice::TaskStage<ice::render::CommandBuffer> frame_transfer;
        ice::TaskStage<> frame_end;
    };

    struct GfxRunner
    {
        virtual ~GfxRunner() noexcept = default;

        virtual void on_resume() noexcept { }

        virtual auto draw_frame(
            ice::EngineFrame const& frame,
            ice::gfx::GfxGraphRuntime& render_graph,
            ice::Clock const& clock
        ) noexcept -> ice::Task<> = 0;

        virtual void on_suspend() noexcept { }

        virtual void final_update(
            ice::EngineFrame const& frame,
            ice::gfx::GfxGraphRuntime& graph_runtime,
            ice::Clock const& clock
        ) noexcept = 0;

        virtual auto device() noexcept -> ice::gfx::GfxDevice& = 0;
    };

} // namespace ice::gfx
