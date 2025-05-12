/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/gfx/gfx_shards.hxx>
#include <ice/gfx/gfx_types.hxx>
#include <ice/task_types.hxx>

namespace ice::gfx
{

    struct GfxRunnerCreateInfo
    {
        ice::Engine& engine;
        ice::render::RenderDriver& driver;
        ice::render::RenderSurface& surface;
        ice::Span<ice::gfx::GfxQueueDefinition const> render_queues;
        ice::TaskScheduler& gfx_thread;
    };

    //! \brief Graphics runner (runtime) object. Handles graphics work between frames and handles swapchain states.
    struct GfxRunner
    {
        virtual ~GfxRunner() noexcept = default;

        //! \brief Sets a rendergraph to used for execution each frame.
        virtual void update_rendergraph(ice::UniquePtr<ice::gfx::GfxGraphRuntime> rendergraph) noexcept = 0;

        virtual auto update_data(
            ice::EngineFrame& frame,
            ice::Clock const& clock
        ) noexcept -> ice::Task<> = 0;

        //! \brief Creates a new draw task for the currently associated render graph.
        virtual auto draw_frame(
            ice::EngineFrame const& frame,
            ice::Clock const& clock
        ) noexcept -> ice::Task<> = 0;

        //! \returns Graphics context associated with this runner.
        virtual auto context() noexcept -> ice::gfx::GfxContext& = 0;

    public:
        virtual void on_resume() noexcept = 0;
        virtual void on_suspend() noexcept = 0;
    };

    //! \warning Might become deprecated, since in almost all cases queues are handled by the engine anyway.
    struct GfxQueueDefinition
    {
        ice::StringID name;
        ice::render::QueueFlags flags;
    };

} // namespace ice::gfx
