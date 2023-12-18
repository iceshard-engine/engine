/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/ecs/ecs_types.hxx>
#include <ice/engine_types.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/asset_storage.hxx>
#include <ice/world/world_trait.hxx>

namespace ice
{

    struct EngineCreateInfo
    {
        ice::UniquePtr<ice::AssetStorage> assets;
        ice::UniquePtr<ice::TraitArchive> traits;
    };

    struct Engine
    {
        virtual ~Engine() noexcept = default;

        virtual auto assets() noexcept -> ice::AssetStorage& = 0;
        virtual auto worlds() noexcept -> ice::WorldAssembly& = 0;
        virtual auto worlds_updater() noexcept -> ice::WorldUpdater& = 0;
        virtual auto entities() noexcept -> ice::ecs::EntityIndex& = 0;
    };

} // namespace ice
