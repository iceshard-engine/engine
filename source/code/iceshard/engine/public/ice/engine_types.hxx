/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task_types.hxx>

namespace ice
{

    struct Engine;
    struct EngineCreateInfo;
    struct EngineFrame;
    struct EngineFrameData;
    struct EngineFrameSchedulers;
    struct EngineFrameUpdate;
    struct EngineRunner;
    struct EngineRunnerCreateInfo;
    struct EngineWorldUpdate;

    struct EngineSchedulers
    {
        ice::TaskScheduler& main;
        ice::TaskScheduler& io;
        ice::TaskScheduler& tasks;
        ice::TaskScheduler& long_tasks;
    };

    struct World;
    struct WorldAssembly;
    struct WorldUpdater;

    struct Trait;
    struct TraitArchive;

    struct DataStorage;

    class EngineDevUI;

    namespace gfx
    {

        class GfxDevice;
        class IceGfxQueueGroup;

        namespace v2
        {

            struct GfxStart;
            struct GfxTransfer;
            struct GfxExecute;
            struct GfxEnd;

            struct GfxStage;
            struct GfxFrame;
            struct GfxRunner;
            struct GfxRenderGraph;
            struct GfxRenderGraphRuntime;
            struct GfxStageDefinition;
            struct GfxObjectStorage;

            // Not final if part of the API
            struct GfxQueueGroup_Temp;
            struct QueueDefinition;

        } // namespace v2

        namespace v3
        {

            struct GfxGraph;
            struct GfxGraphRuntime;

        } // namespace v3

    } // namespace gfx

} // namespace ice
