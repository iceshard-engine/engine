/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/render/render_declarations.hxx>
#include <ice/engine_types.hxx>
#include <ice/task_stage.hxx>

namespace ice::gfx
{

    struct GfxContext;
    struct GfxFrameStages;
    struct RenderFrameUpdate;
    struct GfxGraph;
    struct GfxGraphRuntime;
    struct GfxQueueDefinition;
    struct GfxRunner;
    struct GfxRunnerCreateInfo;
    struct GfxStage;
    struct GfxStageRegistry;
    struct GfxStateChange;

    struct GfxFrameStages
    {
        ice::TaskScheduler& scheduler;
        ice::TaskStage<ice::render::CommandBuffer> frame_transfer;
        ice::TaskStage<> frame_end;
    };

    namespace v2
    {
        struct GfxQueueGroup_Temp;
    }

} // namespace ice::gfx
