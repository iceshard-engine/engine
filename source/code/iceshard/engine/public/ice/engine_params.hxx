/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/engine_types.hxx>
#include <ice/gfx/gfx_types.hxx>

namespace ice
{

    enum class TraitTaskType : ice::u8
    {
        Invalid,

        //! \brief First simulation stage. Updating all logic and data.
        Logic,

        //! \brief Second simulation stage. Updating data / render objects only, no command buffer access.
        Graphics,

        //! \brief Final simulation stage. Recording render command buffers and executing on the GPU.
        Render,
    };

    struct EngineParamsBase
    {
        ice::TraitTaskType task_type;
    };

    struct LogicTaskParams : EngineParamsBase
    {
        ice::Clock const& clock;
        ice::ResourceTracker& resources;
        ice::AssetStorage& assets;
        ice::TaskScheduler& scheduler;
    };

    struct GfxTaskParams : EngineParamsBase
    {
        ice::Clock const& clock;
        ice::ResourceTracker& resources;
        ice::AssetStorage& assets;
        ice::TaskScheduler& scheduler;
        ice::gfx::GfxContext& gfx;
    };

    struct RenderTaskParams : EngineParamsBase
    {
        ice::Clock const& clock;
        ice::ResourceTracker& resources;
        ice::AssetStorage& assets;
        ice::TaskScheduler& scheduler;
        ice::gfx::GfxContext& gfx;
    };

} // namespace ice
