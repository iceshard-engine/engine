/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/clock.hxx>
#include <ice/shard.hxx>
#include <ice/expected.hxx>
#include <ice/engine_types.hxx>


namespace ice
{

    struct World;
    struct WorldAssembly;
    struct WorldStateParams;
    struct WorldUpdater;

    struct WorldStateParams
    {
        ice::Clock const& clock;
        ice::AssetStorage& assets;
        ice::Engine& engine;
        ice::EngineSchedulers thread;
        ice::World& world;
    };

} // namespace ice
