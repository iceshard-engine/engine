/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task_types.hxx>

namespace ice
{

    struct DataStorage;

    struct Engine;
    struct EngineCreateInfo;
    struct EngineDevUI;
    struct EngineFrame;
    struct EngineFrameData;
    struct EngineFrameSchedulers;
    struct EngineFrameUpdate;
    struct EngineRunner;
    struct EngineRunnerCreateInfo;
    struct EngineStateTracker;

    struct EngineSchedulers
    {
        ice::TaskScheduler& main;
        ice::TaskScheduler& tasks;
    };

    struct Trait;
    struct TraitArchive;
    struct TraitDescriptor;
    struct TraitTaskRegistry;

    struct World;
    struct WorldAssembly;
    struct WorldStateParams;
    struct WorldUpdater;

} // namespace ice
