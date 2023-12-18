/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/engine.hxx>
#include <ice/asset_storage.hxx>
#include <ice/world/world_manager.hxx>
#include <ice/ecs/ecs_entity_index.hxx>

#include "iceshard_world_v2.hxx"

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

        void destroy() noexcept;

        auto world_manager() noexcept -> ice::IceshardWorldManager& { return _worlds; }

    private:
        ice::Allocator& _allocator;

        ice::UniquePtr<ice::AssetStorage> _assets;
		ice::IceshardWorldManager _worlds;
        ice::ecs::EntityIndex _entities;
    };

} // namespace ice::v2
