/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
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
        ice::UniquePtr<ice::EngineStateTracker> states;
        ice::UniquePtr<ice::ecs::ArchetypeIndex> archetypes;
        ice::UniquePtr<ice::InputActionStack> action_stack;
    };

    struct Engine
    {
        virtual ~Engine() noexcept = default;

        virtual auto assets() noexcept -> ice::AssetStorage& = 0;
        virtual auto worlds() noexcept -> ice::WorldAssembly& = 0;
        virtual auto worlds_updater() noexcept -> ice::WorldUpdater& = 0;
        virtual auto states() noexcept -> ice::EngineStateTracker& = 0;

        virtual auto entity_archetypes() noexcept -> ice::ecs::ArchetypeIndex& = 0;
        virtual auto entity_index() noexcept -> ice::ecs::EntityIndex& = 0;
        virtual auto entity_storage() noexcept -> ice::ecs::EntityStorage& = 0;

        virtual auto actions() noexcept -> ice::InputActionStack& = 0;
    };

} // namespace ice
