/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/engine.hxx>
#include <ice/engine_state_tracker.hxx>
#include <ice/engine_state_definition.hxx>
#include <ice/asset_storage.hxx>
#include <ice/world/world_updater.hxx>
#include <ice/ecs/ecs_archetype_index.hxx>
#include <ice/ecs/ecs_entity_storage.hxx>

#include "iceshard_world_manager.hxx"

namespace ice
{

    class IceshardEngine final : public ice::Engine
    {
    public:
        IceshardEngine(
            ice::Allocator& alloc,
            ice::EngineCreateInfo create_info
        ) noexcept;

        auto assets() noexcept -> ice::AssetStorage& override;
        auto worlds() noexcept -> ice::WorldAssembly& override;
        auto worlds_updater() noexcept -> ice::WorldUpdater& override;
        auto states() noexcept -> ice::EngineStateTracker& override { return *_states; }

        auto entity_archetypes() noexcept -> ice::ecs::ArchetypeIndex& override { return *_entity_archetypes; }
        auto entity_index() noexcept -> ice::ecs::EntityIndex& override { return _entity_storage.entities(); }
        auto entity_storage() noexcept -> ice::ecs::EntityStorage& override { return _entity_storage; }

        void destroy() noexcept;

        auto world_manager() noexcept -> ice::IceshardWorldManager& { return _worlds; }

    private:
        ice::Allocator& _allocator;

        ice::UniquePtr<ice::AssetStorage> _assets;
        ice::UniquePtr<ice::EngineStateTracker> _states;
        ice::UniquePtr<ice::ecs::ArchetypeIndex> _entity_archetypes;
        ice::ecs::EntityStorage _entity_storage;
        ice::IceshardWorldManager _worlds;
    };

} // namespace ice
