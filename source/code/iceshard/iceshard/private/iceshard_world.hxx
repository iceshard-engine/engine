/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/world/world.hxx>
#include <ice/ecs/ecs_entity_operations.hxx>
#include <ice/devui_widget.hxx>

#include "iceshard_world_tasks_launcher.hxx"
#include "iceshard_world_context.hxx"
#include "iceshard_trait_context.hxx"

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
            ice::ecs::EntityIndex& entities,
            ice::ecs::EntityStorage& entity_storage,
            ice::IceshardWorldContext& context,
            ice::Array<ice::UniquePtr<ice::IceshardTraitContext>> traits,
            ice::detail::TraitTaskTracker* task_tracker
        ) noexcept;
        ~IceshardWorld() noexcept = default;

        auto trait(ice::StringID_Arg trait_identifier) noexcept -> ice::Trait* override { return nullptr; }
        auto trait(ice::StringID_Arg trait_identifier) const noexcept -> ice::Trait const* override { return nullptr; }
        auto trait_storage(ice::Trait* trait) noexcept -> ice::DataStorage* override { return nullptr; }
        auto trait_storage(ice::Trait const* trait) const noexcept -> ice::DataStorage const* override { return nullptr; }

        auto entities() noexcept -> ice::ecs::EntityIndex& override { return _entities; }
        auto entity_queries() noexcept -> ice::ecs::QueryProvider const& override { return _entity_storage; }
        auto entity_queries_storage() noexcept -> ice::ecs::QueryStorage& override { return _entity_query_storage;}
        auto entity_operations() noexcept -> ice::ecs::EntityOperations& override { return _entity_operations; }

        void pre_update(ice::ShardContainer& out_shards) noexcept;

        auto task_launcher() noexcept -> ice::IceshardTasksLauncher& { return _tasks_launcher; }

        auto activate(ice::WorldStateParams const& update) noexcept -> ice::Task<> override;
        auto deactivate(ice::WorldStateParams const& update) noexcept -> ice::Task<> override;

        class DevUI;
        auto devui() noexcept -> DevUI&;

    private:
        ice::ProxyAllocator _allocator;
        ice::ecs::EntityIndex& _entities;
        ice::ecs::EntityStorage& _entity_storage;
        ice::ecs::QueryStorage _entity_query_storage;
        ice::ecs::EntityOperations _entity_operations;
        ice::Array<ice::UniquePtr<ice::IceshardTraitContext>> _traits;
        ice::IceshardTasksLauncher _tasks_launcher;

    private:
        ice::UniquePtr<DevUI> _devui;
        auto create_devui(
            ice::Allocator& alloc,
            ice::IceshardWorldContext& context
        ) noexcept -> ice::UniquePtr<DevUI>;
    };

} // namespace ice
