/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/world/world.hxx>
#include <ice/ecs/ecs_entity_operations.hxx>
#include "iceshard_world_tasks_launcher.hxx"

namespace ice
{

    class IceshardWorld final : public ice::World
    {
    public:
        ice::StringID const worldID;

    public:
        IceshardWorld(
            ice::Allocator& alloc,
            ice::StringID_Arg worldid,
            ice::ecs::EntityStorage& entity_storage,
            ice::Array<ice::UniquePtr<ice::Trait>, ice::ContainerLogic::Complex> traits
        ) noexcept;

        auto trait(ice::StringID_Arg trait_identifier) noexcept -> ice::Trait* override { return nullptr; }
        auto trait(ice::StringID_Arg trait_identifier) const noexcept -> ice::Trait const* override { return nullptr; }
        auto trait_storage(ice::Trait* trait) noexcept -> ice::DataStorage* override { return nullptr; }
        auto trait_storage(ice::Trait const* trait) const noexcept -> ice::DataStorage const* override { return nullptr; }

        auto entity_queries() noexcept -> ice::ecs::QueryProvider& override { return _entity_storage; }
        auto entity_operations() noexcept -> ice::ecs::EntityOperations& override { return _entity_operations; }

        void apply_entity_operations(ice::ShardContainer& out_shards) noexcept;

        auto task_launcher() noexcept -> ice::IceshardTasksLauncher& { return _tasks_launcher; }

        auto activate(ice::WorldStateParams const& update) noexcept -> ice::Task<> override;
        auto deactivate(ice::WorldStateParams const& update) noexcept -> ice::Task<> override;

    private:
        ice::IceshardTasksLauncher _tasks_launcher;
        ice::ecs::EntityStorage& _entity_storage;
        ice::ecs::EntityOperations _entity_operations;
        ice::Array<ice::UniquePtr<ice::Trait>, ice::ContainerLogic::Complex> _traits;
    };

} // namespace ice
