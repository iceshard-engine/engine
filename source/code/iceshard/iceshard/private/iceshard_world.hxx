/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/world/world.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/world/world_updater.hxx>
#include <ice/world/world_trait_archive.hxx>
#include <ice/container/array.hxx>
#include <ice/container/queue.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_state_tracker.hxx>
#include <ice/engine_state_definition.hxx>
#include <ice/sync_manual_events.hxx>
#include <ice/clock.hxx>
#include <ice/mem_allocator_proxy.hxx>

namespace ice
{

    static constexpr ice::ShardID ShardID_PeriodicUpdate = "event/runner/periodic-update"_shardid;

    struct IceshardEventHandler
    {
        ice::ShardID event_id;
        ice::Trait* trait;
        ice::TraitIndirectTaskFn event_handler;
        void* userdata;
    };

    static_assert(sizeof(IceshardEventHandler) == 32);

    class IceshardTraitTaskLauncher final : public ice::TraitTaskRegistry
    {
    public:
        IceshardTraitTaskLauncher(
            ice::Trait* trait,
            ice::HashMap<ice::IceshardEventHandler>& frame_handlers,
            ice::HashMap<ice::IceshardEventHandler>& runner_handlers
        ) noexcept;

        void bind(ice::TraitTaskBinding const& binding) noexcept override;

    private:
        ice::Trait* _trait;
        ice::HashMap<ice::IceshardEventHandler>& _frame_handlers;
        ice::HashMap<ice::IceshardEventHandler>& _runner_handlers;
    };

    class IceshardTasksLauncher
    {
    public:
        IceshardTasksLauncher(ice::Allocator& alloc) noexcept;

        void gather(ice::TaskContainer& out_tasks, ice::Shard shard) noexcept;
        void gather(ice::TaskContainer& out_tasks, ice::Span<ice::Shard const> shards) noexcept;

        void execute(ice::Array<ice::Task<>, ice::ContainerLogic::Complex>& out_tasks, ice::Shard shard) noexcept;
        void execute(ice::Array<ice::Task<>, ice::ContainerLogic::Complex>& out_tasks, ice::ShardContainer const& shards) noexcept;

        auto trait_launcher(ice::Trait* trait) noexcept -> ice::IceshardTraitTaskLauncher;

    private:
        ice::HashMap<IceshardEventHandler> _frame_handlers;
        ice::HashMap<IceshardEventHandler> _runner_handlers;
    };

    class IceshardWorld final : public ice::World
    {
    public:
        IceshardWorld(
            ice::Allocator& alloc,
            ice::StringID_Arg worldid,
            ice::Array<ice::UniquePtr<ice::Trait>, ice::ContainerLogic::Complex> traits
        ) noexcept;

        auto trait(ice::StringID_Arg trait_identifier) noexcept -> ice::Trait* override { return nullptr; }
        auto trait(ice::StringID_Arg trait_identifier) const noexcept -> ice::Trait const* override { return nullptr; }
        auto trait_storage(ice::Trait* trait) noexcept -> ice::DataStorage* override { return nullptr; }
        auto trait_storage(ice::Trait const* trait) const noexcept -> ice::DataStorage const* override { return nullptr; }

        auto task_launcher() noexcept -> ice::IceshardTasksLauncher& { return _tasks_launcher; }

        auto activate(ice::WorldStateParams const& update) noexcept -> ice::Task<> override;
        auto deactivate(ice::WorldStateParams const& update) noexcept -> ice::Task<> override;

        ice::StringID const worldID;
    private:
        ice::IceshardTasksLauncher _tasks_launcher;
        ice::Array<ice::UniquePtr<ice::Trait>, ice::ContainerLogic::Complex> _traits;
    };

    class IceshardWorldManager final
        : public ice::WorldAssembly
        , public ice::WorldUpdater
        , public ice::EngineStateCommitter
    {
        struct Entry;
    public:
        IceshardWorldManager(
            ice::Allocator& alloc,
            ice::UniquePtr<ice::TraitArchive> trait_archive,
            ice::EngineStateTracker& state_tracker
        ) noexcept;
        ~IceshardWorldManager() noexcept;

        auto create_world(ice::WorldTemplate const& world_template) noexcept -> World* override;
        auto find_world(ice::StringID_Arg name) noexcept -> World* override;
        void destroy_world(ice::StringID_Arg name) noexcept override;

        void query_worlds(ice::Array<ice::StringID>& out_worlds) const noexcept override;

        void query_pending_events(
            ice::ShardContainer& out_events
        ) noexcept override;

        void update(
            ice::TaskContainer& out_tasks,
            ice::WorldUpdateParams const& params
        ) noexcept override;

        auto begin() noexcept { return ice::hashmap::begin(_worlds); }
        auto end() noexcept { return ice::hashmap::end(_worlds); }

    public: // Implementation of: ice::EngineStateCommiter
        bool commit(
            ice::EngineStateTrigger const& trigger,
            ice::Shard trigger_shard,
            ice::ShardContainer& out_shards
        ) noexcept override;

    private:
        ice::ProxyAllocator _allocator;
        ice::UniquePtr<ice::TraitArchive> const _trait_archive;
        ice::EngineStateTracker& _state_tracker;

        struct Entry
        {
            ice::UniquePtr<ice::IceshardWorld> world;
            bool is_active;
        };

    private:
        ice::HashMap<Entry> _worlds;
        ice::ShardContainer _pending_events;
    };

} // namespace ice
