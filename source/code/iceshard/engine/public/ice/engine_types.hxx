/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/interfaces.hxx>
#include <ice/task_types.hxx>
#include <ice/asset_types.hxx>

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

} // namespace ice
