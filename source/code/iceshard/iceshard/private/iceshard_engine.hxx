/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/engine.hxx>
#include <ice/engine_state_definition.hxx>
#include <ice/asset_storage.hxx>
#include <ice/world/world_updater.hxx>
#include <ice/ecs/ecs_entity_index.hxx>

#include "iceshard_world.hxx"

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
        auto entities() noexcept -> ice::ecs::EntityIndex& override;
        auto states() noexcept -> ice::EngineStateTracker& override { return *_states; }

        void destroy() noexcept;

        auto world_manager() noexcept -> ice::IceshardWorldManager& { return _worlds; }

    private:
        ice::Allocator& _allocator;

        ice::UniquePtr<ice::AssetStorage> _assets;
        ice::UniquePtr<ice::EngineStateTracker> _states;
        ice::IceshardWorldManager _worlds;
        ice::ecs::EntityIndex _entities;
    };

} // namespace ice
