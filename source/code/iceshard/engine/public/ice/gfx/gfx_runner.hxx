/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/gfx/gfx_shards.hxx>
#include <ice/task_types.hxx>

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

    struct GfxOperationParams
    {
        ice::Clock const& clock;
        ice::EngineFrame const& frame;
        //ice::gfx::GfxGraphRuntime& render_graph;
    };

    struct GfxRunner
    {
        virtual ~GfxRunner() noexcept = default;

        virtual void update_rendergraph(ice::UniquePtr<ice::gfx::GfxGraphRuntime> rendergraph) noexcept = 0;

        virtual void update_states(
            ice::WorldStateTracker& state_tracker,
            ice::gfx::GfxOperationParams const& params
        ) noexcept = 0;

        virtual auto draw_frame(
            ice::gfx::GfxOperationParams const& params
        ) noexcept -> ice::Task<> = 0;

        virtual void on_resume() noexcept { }
        virtual void on_suspend() noexcept { }

        virtual auto device() noexcept -> ice::gfx::GfxDevice& = 0;
    };

} // namespace ice::gfx
