/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
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

    struct EngineSchedulers
    {
        ice::TaskScheduler& main;
        ice::TaskScheduler& tasks;
    };

    struct EngineStateTracker;

    struct World;
    struct WorldAssembly;
    struct WorldUpdater;
    struct WorldStateTracker;
    struct WorldStateParams;

    struct Trait;
    struct TraitArchive;
    struct TraitDescriptor;
    struct TraitTaskRegistry;

    struct DataStorage;

    class EngineDevUI;

} // namespace ice
